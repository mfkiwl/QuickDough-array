
 PARAMETER VERSION = 2.2.0


BEGIN OS
 PARAMETER OS_NAME = device_tree
 PARAMETER PROC_INSTANCE = ps7_cortexa9_0
 PARAMETER console_device = ps7_uart_1
 PARAMETER main_memory = ps7_ddr_0
END


BEGIN PROCESSOR
 PARAMETER DRIVER_NAME = cpu_cortexa9
 PARAMETER HW_INSTANCE = ps7_cortexa9_0
END


BEGIN DRIVER
 PARAMETER DRIVER_NAME = generic
 PARAMETER HW_INSTANCE = acc_ip_0
 PARAMETER compatible = xlnx,acc-ip-1.0
 PARAMETER interrupt-parent = intc
 PARAMETER interrupts = 0 30 1
 PARAMETER reg = 0x43c10000 0x10000
 PARAMETER xlnx,s00-axi-addr-width = 4
 PARAMETER xlnx,s00-axi-data-width = 32
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = generic
 PARAMETER HW_INSTANCE = ps7_afi_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = generic
 PARAMETER HW_INSTANCE = ps7_afi_1
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = generic
 PARAMETER HW_INSTANCE = ps7_afi_2
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = generic
 PARAMETER HW_INSTANCE = ps7_afi_3
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = generic
 PARAMETER HW_INSTANCE = ps7_coresight_comp_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = ddrps
 PARAMETER HW_INSTANCE = ps7_ddr_0
 PARAMETER reg = 0x0 0x20000000
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = ddrcps
 PARAMETER HW_INSTANCE = ps7_ddrc_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = devcfg
 PARAMETER HW_INSTANCE = ps7_dev_cfg_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = none
 PARAMETER HW_INSTANCE = ps7_dma_ns
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = dmaps
 PARAMETER HW_INSTANCE = ps7_dma_s
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = emacps
 PARAMETER HW_INSTANCE = ps7_ethernet_0
 PARAMETER phy-mode = rgmii-id
 PARAMETER xlnx,ptp-enet-clock = 111111115
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = globaltimerps
 PARAMETER HW_INSTANCE = ps7_globaltimer_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = gpiops
 PARAMETER HW_INSTANCE = ps7_gpio_0
 PARAMETER emio-gpio-width = 64
 PARAMETER gpio-mask-high = 0
 PARAMETER gpio-mask-low = 22016
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = generic
 PARAMETER HW_INSTANCE = ps7_gpv_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = generic
 PARAMETER HW_INSTANCE = ps7_intc_dist_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = generic
 PARAMETER HW_INSTANCE = ps7_iop_bus_config_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = generic
 PARAMETER HW_INSTANCE = ps7_l2cachec_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = ocmcps
 PARAMETER HW_INSTANCE = ps7_ocmc_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = pl310ps
 PARAMETER HW_INSTANCE = ps7_pl310_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = pmups
 PARAMETER HW_INSTANCE = ps7_pmu_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = qspips
 PARAMETER HW_INSTANCE = ps7_qspi_0
 PARAMETER is-dual = 0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = generic
 PARAMETER HW_INSTANCE = ps7_qspi_linear_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = ramps
 PARAMETER HW_INSTANCE = ps7_ram_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = none
 PARAMETER HW_INSTANCE = ps7_ram_1
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = generic
 PARAMETER HW_INSTANCE = ps7_scuc_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = scugic
 PARAMETER HW_INSTANCE = ps7_scugic_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = scutimer
 PARAMETER HW_INSTANCE = ps7_scutimer_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = scuwdt
 PARAMETER HW_INSTANCE = ps7_scuwdt_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = generic
 PARAMETER HW_INSTANCE = ps7_sd_0
 PARAMETER xlnx,has-cd = 1
 PARAMETER xlnx,has-power = 0
 PARAMETER xlnx,has-wp = 1
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = slcrps
 PARAMETER HW_INSTANCE = ps7_slcr_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = ttcps
 PARAMETER HW_INSTANCE = ps7_ttc_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = uartps
 PARAMETER HW_INSTANCE = ps7_uart_1
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = usbps
 PARAMETER HW_INSTANCE = ps7_usb_0
 PARAMETER phy_type = ulpi
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = xadcps
 PARAMETER HW_INSTANCE = ps7_xadc_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = generic
 PARAMETER HW_INSTANCE = usr_dma_0
 PARAMETER compatible = xlnx,usr-dma-1.0
 PARAMETER interrupt-parent = intc
 PARAMETER interrupts = 0 29 4
 PARAMETER reg = 0x43c00000 0x10000
END


