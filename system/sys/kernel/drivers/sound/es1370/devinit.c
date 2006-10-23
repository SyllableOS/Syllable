#include "es1370.h"

static void *es1370_driver_data;

int es1370_init(PCI_Info_s *pcidev)
{
	struct es1370_state *s;
//	int i, val, res = -1;
	unsigned int cssr;
	
	if (!(s = kmalloc(sizeof(struct es1370_state), MEMF_KERNEL))) {
		printk(KERN_WARNING "es1370: out of memory\n");
		return -ENOMEM;
	}
	memset(s, 0, sizeof(struct es1370_state));

	s->open_sem = create_semaphore("es1370_open_sem", 1, 0);
        s->sem = create_semaphore("es1370_sem", 1, 0);
	spinlock_init(&s->lock, "es1370_lock");
	s->dev = pcidev;
	s->io = pcidev->u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK;
	s->irq = pcidev->u.h0.nInterruptLine;

        s->dma_dac1.wait = create_semaphore("es1370_dac1_sem", 1, 0);
        s->dma_dac2.wait = create_semaphore("es1370_dac2_sem", 1, 0);
        s->dma_adc.wait = create_semaphore("es1370_adc_sem", 1, 0);
		
        if (request_irq (s->dev->u.h0.nInterruptLine, es1370_interrupt, NULL, SA_SHIRQ, "es1370", s)<0) {
		printk ("unable to obtain IRQ %d, aborting\n",
			s->dev->u.h0.nInterruptLine);
		printk ("EXIT, returning -EBUSY\n");
		return -EBUSY;
	}
   
	printk("found es1370 at io %#lx irq %u\n", s->io, s->irq);
	
	/* initialize codec registers */
	s->sctrl = 0;
	cssr = 0;

	s->ctrl = CTRL_CDC_EN | (DAC2_SRTODIV(44100) << CTRL_SH_PCLKDIV) | (1 << CTRL_SH_WTSRSEL);

	/* initialize the chips */
	outl(s->ctrl, s->io+ES1370_REG_CONTROL);
	outl(s->sctrl, s->io+ES1370_REG_SERIAL_CONTROL);

	/* point phantom write channel to "bugbuf" */
/*	outl((ES1370_REG_PHANTOM_FRAMEADR >> 8) & 15, s->io+ES1370_REG_MEMPAGE);
	outl(virt_to_bus(bugbuf), s->io+(ES1370_REG_PHANTOM_FRAMEADR & 0xff));
	outl(0, s->io+(ES1370_REG_PHANTOM_FRAMECNT & 0xff));*/
	//pci_set_master(pcidev);  /* enable bus mastering */
	wrcodec(s, 0x16, 3); /* no RST, PD */
	wrcodec(s, 0x17, 0); /* CODEC ADC and CODEC DAC use {LR,B}CLK2 and run off the LRCLK2 PLL; program DAC_SYNC=0!!  */
	wrcodec(s, 0x18, 0); /* recording source is mixer */
	wrcodec(s, 0x19, s->mix.micpreamp = 1); /* turn on MIC preamp */
	s->mix.imix = 1;
//	val = SOUND_MASK_LINE|SOUND_MASK_SYNTH|SOUND_MASK_CD;
//	mixer_ioctl(s, SOUND_MIXER_WRITE_RECSRC, (unsigned long)&val);
/*	for (i = 0; i < sizeof(initvol)/sizeof(initvol[0]); i++) {
		val = initvol[i].vol;
		mixer_ioctl(s, initvol[i].mixch, (unsigned long)&val);
	}*/
/*
	outl(0, s->io+ES1371_REG_LEGACY);*/
	/* init the sample rate converter */
//	src_init(s);
	/* set default values */

/*	val = SOUND_MASK_LINE;
	mixdev_ioctl(&s->codec, SOUND_MIXER_WRITE_RECSRC, (unsigned long)&val);
	for (i = 0; i < sizeof(initvol)/sizeof(initvol[0]); i++) {
		val = initvol[i].vol;
		mixdev_ioctl(&s->codec, initvol[i].mixch, (unsigned long)&val);
	}*/
	
        es1370_driver_data = s;
//        s->open = false;
   
       	return 0;

/* error:
	kfree(s);
	return res;*/
}

DeviceOperations_s es1370_dsp_fops = {
		es1370_open,
		es1370_release,
		es1370_ioctl,
		NULL,
		es1370_write
};

status_t device_init( int nDeviceID )
{
	int i;
	bool found = false;
	PCI_Info_s pci;
	PCI_bus_s* psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	if( psBus == NULL )
		return( -ENODEV );

	printk ("Ensoniq 1370 driver\n");
	
	

	/* scan all PCI devices */
	for(i = 0 ;  psBus->get_pci_info( &pci, i ) == 0 && found != true ; ++i) {
		if ( (pci.nVendorID == PCI_VENDOR_ID_ENSONIQ && pci.nDeviceID == PCI_DEVICE_ID_ENSONIQ_ES1370) ) {
			if(es1370_init(&pci) == 0) {
				if( claim_device( nDeviceID, pci.nHandle, "Ensoniq ES1370", DEVICE_AUDIO ) == 0 ) {
					found = true;
					break;
				}
			}
        }
	}

	if(found) {
    	/* create DSP node */
		if( create_device_node( nDeviceID, pci.nHandle, "audio/es1370", &es1370_dsp_fops, es1370_driver_data ) < 0 ) {
			printk( "Failed to create dsp node \n");
			return ( -EINVAL );
		}
	        /* create mixer node */
		/*if( create_device_node( nDeviceID, "audio/mixer/es1370", &es1370_mixer_fops, es1370_driver_data ) < 0 ) {
			printk( "Failed to create mixer node \n");
			return ( -EINVAL );
		}*/
		printk( "Found!\n" );
	} else {
		printk( "No device found\n" );
		disable_device( nDeviceID );
		return ( -EINVAL );
	}
	return (0);
}

int device_uninit( int nDeviceID )
{
        struct es1370_state *s = (struct es1370_state *)es1370_driver_data;

	printk( "es1370 uninit\n" );
	delete_semaphore( s->open_sem );
        delete_semaphore( s->sem );
        delete_semaphore( s->dma_dac1.wait );
        delete_semaphore( s->dma_dac2.wait );
        delete_semaphore( s->dma_adc.wait );
	kfree(s);
	printk( "es1370 clean up completed\n" );
	return 0;
}
















