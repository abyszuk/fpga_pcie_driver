#if !defined(_PCIDRIVER_INT_H) && defined(ENABLE_IRQ)
#define _PCIDRIVER_INT_H

int pcidriver_probe_irq(pcidriver_privdata_t *privdata);
void pcidriver_remove_irq(pcidriver_privdata_t *privdata);
void pcidriver_irq_unmap_bars(pcidriver_privdata_t *privdata);
irqreturn_t pcidriver_irq_handler(int irq, void *dev_id);

#endif
