int pcidriver_umem_sgmap( pcidriver_privdata_t *privdata, umem_handle_t *umem_handle );
int pcidriver_umem_sgunmap( pcidriver_privdata_t *privdata, pcidriver_umem_entry_t *umem_entry );
int pcidriver_umem_sgget( pcidriver_privdata_t *privdata, umem_sglist_t *umem_sglist );
int pcidriver_umem_sync( pcidriver_privdata_t *privdata, umem_handle_t *umem_handle );
pcidriver_umem_entry_t *pcidriver_umem_find_entry_id( pcidriver_privdata_t *privdata, int id );
