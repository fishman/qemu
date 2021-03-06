/*
 *  Low Pin Count emulation
 *
 *  Copyright (c) 2007 Alexander Graf
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * *****************************************************************
 *
 * This driver emulates an ICH-7 LPC partially. The LPC is basically the
 * same as the ISA-bridge in the existing PIIX implementation, but
 * more recent and includes support for HPET and Power Management.
 *
 */
#include "hw.h"
#include "pc.h"
#include "pci.h"
#include "console.h"

#define RCBA_BASE		0xFED1C000

/* void hpet_init(qemu_irq irq); */

static uint32_t rcba_ram_readl(void *opaque, target_phys_addr_t addr)
{
    printf("qemu: rcba_read l at %#lx\n", addr);
    if(addr == RCBA_BASE + 0x3404) { /* This is the HPET config pointer */
        printf("qemu: rcba_read HPET_CONFIG_POINTER\n");
        return 0xf0; // enabled at 0xfed00000
    } else if(addr == RCBA_BASE + 0x3410) { /* This is the HPET config pointer */
        printf("qemu: rcba_read GCS\n");
        return 0;
    } else {
        return 0x0;
    }
}

static void rcba_ram_writel(void *opaque, target_phys_addr_t addr,
                uint32_t value)
{
    printf("qemu: rcba_write l %#lx = %#x\n", addr, value);
}

static CPUReadMemoryFunc *rcba_ram_read[] = {
    NULL,
    NULL,
    rcba_ram_readl,
};

static CPUWriteMemoryFunc *rcba_ram_write[] = {
    NULL,
    NULL,
    rcba_ram_writel,
};

void lpc_init(PCIBus *bus, int devfn, qemu_irq *pic) {
    int iomemtype;
    uint8_t *pci_conf;
    PCIDevice *d;

    /* register a function 1 of PIIX3 */
    d = (PCIDevice *)pci_register_device(bus, "LPC",
                                           sizeof(PCIDevice),
                                           31 << 3,
                                           NULL, NULL);
    pci_conf = d->config;
    pci_conf[0x00] = 0x86;
    pci_conf[0x01] = 0x80;
    pci_conf[0x02] = 0xb9;
    pci_conf[0x03] = 0x27;
    pci_conf[0x08] = 0x02; // Revision 2

    pci_conf[0x0a] = 0x01; // PCI-to-ISA Bridge
    pci_conf[0x0b] = 0x06; // Bridge

    pci_conf[0x0e] = 0xf0;

    // Subsystem
    pci_conf[0x2c] = 0x86;
    pci_conf[0x2d] = 0x80;
    pci_conf[0x2e] = 0x70;
    pci_conf[0x2f] = 0x72;

    pci_conf[0x3d] = 0x03;

    // PMBASE
    pci_conf[0x40] = 0x01;
    pci_conf[0x41] = 0x0b;

    pci_conf[0xf0] = RCBA_BASE | 1; // enabled
    pci_conf[0xf1] = RCBA_BASE << 8;
    pci_conf[0xf2] = RCBA_BASE << 16;
    pci_conf[0xf3] = RCBA_BASE << 24;


    /* RCBA Area */

    iomemtype = cpu_register_io_memory(rcba_ram_read, rcba_ram_write, d,
                                       DEVICE_LITTLE_ENDIAN);

    cpu_register_physical_memory(RCBA_BASE, 0x4000, iomemtype);
#if 0
    cpu_register_physical_memory(0x00CDA000, 0x4000, iomemtype);
#endif



    pci_conf[0x04] = 0x07; // master, memory and I/O
    pci_conf[0x05] = 0x00;
    pci_conf[0x06] = 0x00;
    pci_conf[0x07] = 0x02; // PCI_status_devsel_medium
    pci_conf[0x4c] = 0x4d;
    pci_conf[0x4e] = 0x03;
    pci_conf[0x4f] = 0x00;
    pci_conf[0x60] = 0x0a; // PCI A -> IRQ 10
    pci_conf[0x61] = 0x0a; // PCI B -> IRQ 10
    pci_conf[0x62] = 0x0b; // PCI C -> IRQ 11
    pci_conf[0x63] = 0x0b; // PCI D -> IRQ 11
    pci_conf[0x69] = 0x02;
    pci_conf[0x70] = 0x80;
    pci_conf[0x76] = 0x0c;
    pci_conf[0x77] = 0x0c;
    pci_conf[0x78] = 0x02;
    pci_conf[0x79] = 0x00;
    pci_conf[0x80] = 0x00;
    pci_conf[0x82] = 0x00;
    pci_conf[0xa0] = 0x08;
    pci_conf[0xa2] = 0x00;
    pci_conf[0xa3] = 0x00;
    pci_conf[0xa4] = 0x00;
    pci_conf[0xa5] = 0x00;
    pci_conf[0xa6] = 0x00;
    pci_conf[0xa7] = 0x00;
    pci_conf[0xa8] = 0x0f;
    pci_conf[0xaa] = 0x00;
    pci_conf[0xab] = 0x00;
    pci_conf[0xac] = 0x00;
    pci_conf[0xae] = 0x00;

// XXX hpet goes via apic
    /* hpet_init(pic); */

#if 0
    register_ioport_read(0x1000, 128, 1, pmbase_readb, d);
    register_ioport_write(0x1000, 128, 1, pmbase_writeb, d);
    register_ioport_read(0x1000, 64, 2, pmbase_readw, d);
    register_ioport_write(0x1000, 64, 2, pmbase_writew, d);
    register_ioport_read(0x1000, 32, 4, pmbase_readl, d);
    register_ioport_write(0x1000, 32, 5, pmbase_writel, d);
#endif
}

