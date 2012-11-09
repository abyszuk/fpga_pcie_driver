int pcidriver_kmem_alloc( pcidriver_privdata_t *privdata, kmem_handle_t *kmem_handle );
int pcidriver_kmem_free(  pcidriver_privdata_t *privdata, kmem_handle_t *kmem_handle );
int pcidriver_kmem_sync(  pcidriver_privdata_t *privdata, kmem_sync_t *kmem_sync );
int pcidriver_kmem_free_all(  pcidriver_privdata_t *privdata );
pcidriver_kmem_entry_t *pcidriver_kmem_find_entry( pcidriver_privdata_t *privdata, kmem_handle_t *kmem_handle );
pcidriver_kmem_entry_t *pcidriver_kmem_find_entry_id( pcidriver_privdata_t *privdata, int id );
int pcidriver_kmem_free_entry( pcidriver_privdata_t *privdata, pcidriver_kmem_entry_t *kmem_entry );
