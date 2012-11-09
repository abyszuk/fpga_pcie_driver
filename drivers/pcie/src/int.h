#if !defined(_PCIDRIVER_INT_H) && defined(ENABLE_IRQ)
#define _PCIDRIVER_INT_H

int pcidriver_probe_irq(pcidriver_privdata_t *privdata);
void pcidriver_remove_irq(pcidriver_privdata_t *privdata);
void pcidriver_irq_unmap_bars(pcidriver_privdata_t *privdata);
IRQ_HANDLER_FUNC(pcidriver_irq_handler);

#endif
