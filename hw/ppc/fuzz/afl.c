/*
 * QEMU American Fuzzy Lop PowerPC board
 * Copyright (c) 2019 S. Duverger Airbus
 * GPLv2
 */
#include "qemu/afl.h"

//#define BIOS_SIZE (1024 * 1024)

static void ppc_afl_reset(void *opaque)
{
    PowerPCCPU *cpu = opaque;
    cpu_reset(CPU(cpu));
}

#ifdef AFL_RAM_GUARD
void afl_arch_ram_guard_setup(afl_t *afl)
{
   debug("ramguard: NOT IMPLEMENTED !\n");
}
#endif

void afl_init_arch(afl_t *afl, MachineState *mcs, MemoryRegion *sysmem)
{
   // useless for now, as prep_light_init() is inlined below
   (void)0;
}

static void ppc_prep_light_init(afl_t *afl, MachineState *machine)
{
   PowerPCCPU *cpu = POWERPC_CPU(cpu_create(machine->cpu_type));
   CPUPPCState *env = &cpu->env;
   MemoryRegion *sysmem = get_system_memory();
   MemoryRegion *ram = g_new(MemoryRegion, 1);
   ram_addr_t ram_size = machine->ram_size;

   /* AFL keep track */
   afl->ram_mr = ram;
   afl->arch.cpu = cpu;

   if (env->flags & POWERPC_FLAG_RTC_CLK) {
      /* POWER / PowerPC 601 RTC clock frequency is 7.8125 MHz */
      cpu_ppc_tb_init(env, 7812500UL);
   } else {
      /* Set time-base frequency to 100 Mhz */
      cpu_ppc_tb_init(env, 100UL * 1000UL * 1000UL);
   }
   qemu_register_reset(ppc_afl_reset, cpu);

   /* allocate RAM */
   memory_region_allocate_system_memory(ram, NULL, "ppc_afl.ram", ram_size);
   memory_region_add_subregion(sysmem, 0, ram);



   DeviceState *dev;
   PCIHostState *pcihost;
   PCIBus *pci_bus;
   PCIDevice *pci;
   ISABus *isa_bus;
   ISADevice *isa;


   dev = qdev_create(NULL, "raven-pcihost");
   qdev_prop_set_string(dev, "bios-name", bios_name);
   //qdev_prop_set_uint32(dev, "elf-machine", PPC_ELF_MACHINE);
   qdev_prop_set_bit(dev, "is-legacy-prep", true);
   pcihost = PCI_HOST_BRIDGE(dev);
   object_property_add_child(qdev_get_machine(), "raven", OBJECT(dev), NULL);
   qdev_init_nofail(dev);

   pci_bus = (PCIBus *)qdev_get_child_bus(dev, "pci.0");
   //sysctrl->contiguous_map_irq = qdev_get_gpio_in(dev, 0);

   /* PCI -> ISA bridge */
   pci = pci_create_simple(pci_bus, PCI_DEVFN(1, 0), "i82378");

   sysbus_connect_irq(&pcihost->busdev, 0, qdev_get_gpio_in(&pci->qdev, 9));
   sysbus_connect_irq(&pcihost->busdev, 1, qdev_get_gpio_in(&pci->qdev, 11));
   sysbus_connect_irq(&pcihost->busdev, 2, qdev_get_gpio_in(&pci->qdev, 9));
   sysbus_connect_irq(&pcihost->busdev, 3, qdev_get_gpio_in(&pci->qdev, 11));
   isa_bus = ISA_BUS(qdev_get_child_bus(DEVICE(pci), "isa.0"));

   isa = isa_create(isa_bus, TYPE_PC87312_SUPERIO);
   dev = DEVICE(isa);
   qdev_prop_set_uint8(dev, "config", 13); /* fdc, ser0, ser1, par0 */
   qdev_init_nofail(dev);


   /* /\* BIOS *\/ */
   /* MemoryRegion *bios = g_new(MemoryRegion, 1); */
   /* memory_region_init_ram_nomigrate(bios, NULL, "bios", BIOS_SIZE, &error_fatal); */
   /* memory_region_set_readonly(bios, true); */
   /* memory_region_add_subregion(sysmem, (uint32_t)(-BIOS_SIZE), bios); */

   /* if (!bios_name) { */
   /*    error_report("AFL ppc need bios image name"); */
   /*    exit(EXIT_FAILURE); */
   /* } */

   /* int bios_size = -1; */
   /* char *filename = qemu_find_file(QEMU_FILE_TYPE_BIOS, bios_name); */

   /* if (filename) { */
   /*    bios_size = get_image_size(filename); */
   /*    if (bios_size > 0 && bios_size <= BIOS_SIZE) { */
   /*       hwaddr bios_addr; */
   /*       bios_size = (bios_size + 0xfff) & ~0xfff; */
   /*       bios_addr = (uint32_t)(-BIOS_SIZE); */
   /*       bios_size = load_image_targphys(filename, bios_addr, bios_size); */
   /*    } */
   /* } */

   /* g_free(filename); */

   /* if (bios_size < 0 || bios_size > BIOS_SIZE) { */
   /*    memory_region_del_subregion(sysmem, bios); */
   /*    error_report("AFL ppc could not load bios image"); */
   /*    exit(EXIT_FAILURE); */
   /* } */

   /* vmstate_register_ram_global(bios); */

   /* DeviceState *dev; */
   /* ISABus *isa_bus; */
   /* ISADevice *isa; */

   /* AddressSpace *pci_io_as = g_new(AddressSpace, 1); */
   /* MemoryRegion *pci_io = g_new(MemoryRegion, 1); */

   /* memory_region_init(pci_io, NULL, "pci-io", 0x3f800000); */
   /* address_space_init(pci_io_as, pci_io, "raven-io"); */
   /* memory_region_add_subregion(sysmem, 0x80000000, pci_io); */

   /* isa_bus= isa_bus_new(NULL, sysmem, pci_io, NULL); */
   /* isa = isa_create(isa_bus, TYPE_ISA_SERIAL); //TYPE_PC87312_SUPERIO); */
   /* isa_bus_irqs(isa_bus, g_new0(qemu_irq, ISA_NUM_IRQS)); */

   /* dev = DEVICE(isa); */
   /* qdev_prop_set_uint32(dev, "index", 0); */
   /* qdev_prop_set_uint32(d, "iobase", */
   /*                      k->serial.get_iobase(sio, i)); */

   /* qdev_init_nofail(dev); */
}

static void ppc_afl_init(MachineState *mcs)
{
   afl_t *afl = afl_pre_init();
   ppc_prep_light_init(afl, mcs);
   afl_init(afl, mcs);
}

static void ppc_afl_machine_init(MachineClass *mc)
{
   mc->desc = "PowerPC AFL platform";
   mc->init = ppc_afl_init;
   mc->block_default_type = IF_IDE;
   mc->max_cpus = 1;
   mc->default_boot_order = "cad";
   mc->default_cpu_type = POWERPC_CPU_TYPE_NAME("7400_v2.9");
}

DEFINE_MACHINE("afl", ppc_afl_machine_init)
