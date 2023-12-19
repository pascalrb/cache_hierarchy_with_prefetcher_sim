#ifndef CACHESIM_H
#define CACHESIM_H

// TODO remove
using namespace std;

#include <vector>
#include <assert.h>
#include <math.h>  // log stuff
#include <stdint.h>
#include <cstdlib> //exit() EXIT_FAILURE
#include <algorithm> //sort

#define ADDR_SIZE 32

typedef struct {
   uint32_t blocksize;
   uint32_t l1_size;
   uint32_t l1_assoc;
   uint32_t l2_size;
   uint32_t l2_assoc;
   uint32_t pref_n;
   uint32_t pref_m;
} cache_params_t;


// global variable to count memory access operations
uint32_t g_mem_op_count;                // q

typedef struct {
    bool valid_bit;
    bool dirty_bit;
    uint32_t lru;
    uint32_t tag;
} cache_block;

class Cache {
    private: 
        uint32_t cache_lvl;
        uint32_t assoc; //associativity; num of ways

        uint32_t block_size;
        uint32_t block_bits_num;
        uint32_t sets_num;
        uint32_t index_bits_num;
        uint32_t tag_bits_num;

        vector<vector<cache_block> > blocks;

        vector <uint32_t> prefetch_heads;
        uint32_t pref_n;
        uint32_t pref_m;

        // Initializes the members of the this cache's cache_blocks
        void init_cache_blocks();

        void update_lru(uint32_t op_idx, uint32_t op_lru);
        void make_space_in_set(uint32_t addr);
        void place_block_in_set(uint32_t addr, bool set_dirty_bit);
        bool update_prefetcher(uint32_t block_addr, bool cache_hit);
        void debug_print_cache_set(uint32_t addr, bool is_before, char c);
        void debug_print_prefetcher();


    public: 
        void cache_sets_sort();
        void print_cache();
        void print_prefetcher();

        // TODO: move these counters in private and have public getters for them
        uint32_t read_count;                    // a, h
        uint32_t read_miss_count;               // b, i (not including read cache miss that hit in prefetcher)
        uint32_t write_count;                   // c, l (same as f for L2) 
        uint32_t write_miss_count;              // d, m (not including read cache miss that hit in prefetcher)
        uint32_t writebacks_to_next_lvl_count;  // f, o (to memory for L2 for this sim) 
        uint32_t prefetches_to_next_lvl_count; // g, p 
        uint32_t read_from_prefetch_count;      // j
        uint32_t read_miss_from_prefetch_count; // k

        Cache* next_lvl_cache;

        // Constructor
        Cache(uint32_t cache_lvl, uint32_t lvl_size, uint32_t lvl_assoc, uint32_t block_size, uint32_t pref_n, uint32_t pref_m);
        // TODO destructor 
        //~Cache ();

        void read(uint32_t addr);
        void write(uint32_t addr);
};

// Helper function used only during contructor initialization 
void Cache::init_cache_blocks(){

    for (uint32_t i = 0; i < this->sets_num; i++){
        vector<cache_block> set_of_blocks;

        for (uint32_t j = 0; j < this->assoc; j++){
            cache_block block;

            block.valid_bit = 0;
            block.lru = j;
            //tag and dirty_bit can be garbage initially. 

            set_of_blocks.push_back(block);
        }
        this->blocks.push_back(set_of_blocks);
    }
}

// Constructor 
Cache::Cache(uint32_t cache_lvl, uint32_t lvl_size, uint32_t lvl_assoc, uint32_t block_size, uint32_t pref_n, uint32_t pref_m){
    this->cache_lvl = cache_lvl;
    this->read_count                    = 0;   
    this->read_miss_count               = 0;  
    this->write_count                   = 0;
    this->write_miss_count              = 0;
    this->writebacks_to_next_lvl_count  = 0; 
    this->prefetches_to_next_lvl_count  = 0; 
    this->read_from_prefetch_count      = 0; 
    this->read_miss_from_prefetch_count = 0; 

    if (lvl_size == 0){ // that lvl is disabled
        return;
    }

    this->assoc = lvl_assoc;

    this->block_size = block_size;
    this->block_bits_num = log2(this->block_size);

    this->sets_num       = lvl_size / (this->assoc * this->block_size); 
    this->index_bits_num = log2(this->sets_num);

    this->tag_bits_num   = ADDR_SIZE - (this->index_bits_num + this->block_bits_num);

    this->next_lvl_cache = NULL;

    this->init_cache_blocks();

    this->pref_n = pref_n;
    this->pref_m = pref_m;
    if (this->pref_n > 0){ // prefetch is enabled
        // Initially set to 0 (invalid). Using the order of the heads to denote LRU (0->N = MRU->LRU).
        //
        // Each head represents a set of M blocks; since they are consecutive (distance = 1) then I can
        // use the offset M to "cover" and check for all M blocks from consecutive the head of prefetcher
        for(uint32_t i=0; i<pref_n; i++){
            this->prefetch_heads.push_back(0);
        }
    }
}

bool cache_comp(cache_block l, cache_block r){
    return (l.lru < r.lru);
}
void Cache::cache_sets_sort(){
    for(uint32_t i = 0; i < this->sets_num; i++){
        sort(this->blocks[i].begin(), this->blocks[i].end(), cache_comp);
    }
}

void Cache::debug_print_cache_set(uint32_t addr, bool is_before, char c){
    uint32_t block_addr = addr >> this->block_bits_num;
    uint32_t op_idx     = block_addr % this->sets_num;
    uint32_t op_tag     = block_addr >> this->index_bits_num;

    const char * tab = (this->cache_lvl > 1) ? "\t\t" : "\t";
    if(is_before){
        printf("%sL%d: %c %x (tag=%x index=%d)\n", tab, this->cache_lvl, c, addr, op_tag, op_idx);
        printf("%sL%d: before: set %7d: ", tab, this->cache_lvl, op_idx);
    }else{
        printf("%sL%d:  after: set %7d: ", tab, this->cache_lvl, op_idx);
    }
    for(uint32_t i=0; i<this->assoc; i++){
        if (this->blocks[op_idx][i].valid_bit){ 
            if(this->blocks[op_idx][i].dirty_bit){
                printf("%8x D", this->blocks[op_idx][i].tag);
            }else{
                printf("%8x", this->blocks[op_idx][i].tag);
            }
        }
    }
    printf("\n");
}
void Cache::debug_print_prefetcher(){
    const char * tab = "\t\t\t";
    for(uint32_t i =0; i<this->prefetch_heads.size(); i++){
        if(prefetch_heads[i]){
            printf("%sSB:", tab);
            for(uint32_t j=0; j<this->pref_m; j++){
                printf("%9x", this->prefetch_heads[i]+j);
            }
            printf("\n");
        }
    }
}

void Cache::print_cache(){
    for(uint32_t i = 0; i < this->sets_num; i++){
        printf("set%7d:", i);
        for(uint32_t j=0; j<this->assoc; j++){
            if(this->blocks[i][j].valid_bit){
                //printf("%9x.%d %s", this->blocks[i][j].tag, this->blocks[i][j].lru, ((this->blocks[i][j].dirty_bit) ? "D":" "));
                printf("%9x %s", this->blocks[i][j].tag, ((this->blocks[i][j].dirty_bit) ? "D":" "));
            }else{
                printf("%9s", "");
            }
        }
        printf("\n");
    }
}

// Should only be called if at last lvl and the pref is enabled
void Cache::print_prefetcher(){
    assert(this->pref_n > 0 && this->next_lvl_cache == NULL);

    for(uint32_t i =0; i<this->prefetch_heads.size(); i++){
        if(this->prefetch_heads[i]){
            for(uint32_t j=0; j<this->pref_m; j++){
                printf("%9x", this->prefetch_heads[i]+j);
            }
            printf("\n");
        }
    }
}

// Checks if block is in any of the stream buffers. 
// Also updates prefether LRU according prefetcher hits
// Note: Should only be called when prefetcher at a cache lvl is enabled
bool Cache::update_prefetcher(uint32_t block_addr, bool cache_hit){

    // In this simulation, only last level cache has a prefetcher unit 
    assert(this->next_lvl_cache == NULL);

    // Check each stream heads in multiple stream buffers
    for(uint32_t i = 0; i < this->prefetch_heads.size(); i++){  
                                                // MRU -> LRU = prefetch_heads[0] <- prefetch_heads[pref_n-1]

        // Each head "contains" a set of M blocks; since they are consecutive (distance = 1) then I can
        // use the offset M to "cover" and check for all M blocks from consecutive the head of prefetcher
        if(this->prefetch_heads[i] <= block_addr && block_addr < this->prefetch_heads[i] + this->pref_m){     // prefetch hit
            // Update LRU of prefetcher unit

            //scenario 4 and 2
            this->prefetches_to_next_lvl_count += (block_addr - this->prefetch_heads[i] + 1);
            g_mem_op_count += (block_addr - this->prefetch_heads[i] + 1);

            this->prefetch_heads.erase(this->prefetch_heads.begin() + i);
            this->prefetch_heads.insert(this->prefetch_heads.begin(), block_addr + 1);// Update value of head of MRU stream buffer
                                                             
            assert(this->prefetch_heads.size() == this->pref_n);

            return 1;
        }
    }

    // If here, the block_addr is not in any of the stream buffers
    if (cache_hit){ // scenario 3
        // do nothing

    }else{ // scenario 1
        // In last cache lvl before mem; we can assume hit in mem for r/w
        // In this sumulation, only last level has prefetcher so no need to do a read on the next lvl cache
        // so we directly 'get' it from memory. Otherwise, we could issue a read to next lvl cache with a flag 
        // saying that it's coming from the prefetcher. Then in that instance of read increment read_from_prefetch_count. 
        // If that read missed, increment read_miss_from_prefetch_count. 
        this->prefetches_to_next_lvl_count += this->pref_m;
        g_mem_op_count += this->pref_m;

        this->prefetch_heads.pop_back(); // remove LRU
        this->prefetch_heads.insert(this->prefetch_heads.begin(), block_addr + 1); // Update value of head of MRU stream buffer

        g_mem_op_count++; //cache "issueing a read" to mem because it missed in both cache and prefetcher
    }

    assert(this->prefetch_heads.size() == this->pref_n);
    return 0;
}


// Only increment lru of blocks with lru smaller then the lru of the block getting "touched"
void Cache::update_lru(uint32_t op_idx, uint32_t op_lru){
    for(uint32_t i = 0; i < this->assoc; i++){
        if (this->blocks[op_idx][i].lru < op_lru){
            this->blocks[op_idx][i].lru++;
        }else if (this->blocks[op_idx][i].lru == op_lru){
            this->blocks[op_idx][i].lru = 0;
        }
    }
}

void Cache::make_space_in_set(uint32_t addr){
    uint32_t block_addr = addr >> this->block_bits_num;
    uint32_t op_idx     = block_addr % this->sets_num;

    for (uint32_t i = 0; i < this->assoc; i++){ 
        if (this->blocks[op_idx][i].lru == this->assoc-1){ // At the lru;
                                                           // A given block's lru is the actual LRU 
            if (this->blocks[op_idx][i].valid_bit){
                if(this->blocks[op_idx][i].dirty_bit){//have to evict
                    // block is dirty; need to writeback to next lvl AND update its prefetcher 
                    // (since we don't use the actual value (that's dirty)
                    // in this simulation, no need to update the prefetcher. 
                    // issue write of what's inside that block to next level 
                    if (this->next_lvl_cache == NULL){
                        // "writing to mem"
                        g_mem_op_count++;
                    }else{
                        uint32_t victim_full_addr = this->blocks[op_idx][i].tag << this->index_bits_num;
                        victim_full_addr |= op_idx;
                        victim_full_addr <<= this->block_bits_num; // TODO we lose the block offset information 
                                                                    // (but not used in this simualation)
                        this->next_lvl_cache->write(victim_full_addr);
                    }
                    this->writebacks_to_next_lvl_count++;
                }
            }else{
                //the LRU is empty
            }
            return;
        }
    }
}

void Cache::place_block_in_set(uint32_t addr, bool set_dirty){
    uint32_t block_addr = addr >> this->block_bits_num;
    uint32_t op_idx     = block_addr % this->sets_num;
    uint32_t op_tag     = block_addr >> this->index_bits_num;

    for (uint32_t i = 0; i < this->assoc; i++){ 
        if (this->blocks[op_idx][i].lru == this->assoc-1){ // At the lru;
                                                           // A given block's lru is the actual LRU 
            this->blocks[op_idx][i].tag          = op_tag;
            this->blocks[op_idx][i].valid_bit    = 1;
            this->blocks[op_idx][i].lru          = 0;
            this->blocks[op_idx][i].dirty_bit = (set_dirty) ? 1 : 0;
        }else{
            this->blocks[op_idx][i].lru++;
        }
    }
}

// CPU or upper cache lvl initiated read on 'this' Cache
void Cache::read(uint32_t addr){
    this->read_count++; 

    uint32_t addr1 = addr;
    uint32_t block_addr = addr1 >> this->block_bits_num;
    uint32_t op_idx     = block_addr % this->sets_num;
    uint32_t op_tag     = block_addr >> this->index_bits_num;

    //-----------------printing content ----------------
    //this->debug_print_cache_set(addr, 1, 'r');
    // -------------------------------------------------------

    for (uint32_t i = 0; i < this->assoc; i++){
        if (this->blocks[op_idx][i].valid_bit){
            if (this->blocks[op_idx][i].tag == op_tag){  //read hit

                if(this->next_lvl_cache == NULL && this->pref_n > 0){
                    this->update_prefetcher(block_addr, 1); // scenario 3 and 4
                }
                // update lru
                this->update_lru(op_idx, this->blocks[op_idx][i].lru);

                //-----------------printing content ----------------
                //this->debug_print_cache_set(addr, 0, 'r');
                //this->debug_print_prefetcher();
                // -------------------------------------------------------
                return;
            }
        }
    }

    // If we're here, this lvl miss, need to issue a read on next lvl
    this->read_miss_count++;
    this->make_space_in_set(addr);

    if(this->pref_n > 0){
        if(this->next_lvl_cache == NULL){
            if(this->update_prefetcher(block_addr, 0)){
                this->read_miss_count--;
            }
        }else{
            this->next_lvl_cache->read(addr);
        }

    }else{ // prefetch is disabled; issue read to next level 
        if(this->next_lvl_cache == NULL){
            // "reading from mem"
            g_mem_op_count++;
        }else{
            this->next_lvl_cache->read(addr);
        }
    }

    this->place_block_in_set(addr, 0);

    //-----------------printing content ----------------
    //this->debug_print_cache_set(addr, 0, 'r');
    //this->debug_print_prefetcher();
    // -------------------------------------------------------
}

// CPU or upper cache lvl initiated read on 'this' cache. 
void Cache::write(uint32_t addr){
    this->write_count++; 

    uint32_t addr1 = addr;
    uint32_t block_addr = addr1 >> this->block_bits_num;
    uint32_t op_idx     = block_addr % this->sets_num;
    uint32_t op_tag     = block_addr >> this->index_bits_num;

    //-----------------printing content ----------------
    //this->debug_print_cache_set(addr, 1, 'w');
    // -------------------------------------------------------

    for (uint32_t i = 0; i < this->assoc; i++){
        if (this->blocks[op_idx][i].valid_bit){
            if(this->blocks[op_idx][i].tag == op_tag){ // write hit

                if(this->next_lvl_cache == NULL && this->pref_n > 0){
                    this->update_prefetcher(block_addr, 1); // scenario 3 and 4
                }
                if (!this->blocks[op_idx][i].dirty_bit){ // clean block; simply write on it
                    this->blocks[op_idx][i].dirty_bit = 1;

                }else{ // dirty block
                    // block is already dirty; keep writing on it
                }
                this->update_lru(op_idx, this->blocks[op_idx][i].lru);

                //-----------------printing content ----------------
                //this->debug_print_cache_set(addr, 0, 'w');
                //this->debug_print_prefetcher();
                // -------------------------------------------------------
                return;
            }
        }
    }

    // If we're here, this lvl miss, need to issue a read on next lvl
    this->write_miss_count++;

    //this->allocate_block(addr, 1);
    this->make_space_in_set(addr);

    if(this->pref_n > 0){
        if(this->next_lvl_cache == NULL){
            if(this->update_prefetcher(block_addr, 0)){
                this->write_miss_count--;
            }
        }else{
            this->next_lvl_cache->read(addr);
        }
    }else{ // prefetch is disabled; issue read to next level 
        if(this->next_lvl_cache == NULL){
            // "reading from mem"
            g_mem_op_count++;
        }else{
            this->next_lvl_cache->read(addr);
        }
    }
    this->place_block_in_set(addr, 1);

    //-----------------printing content ----------------
    //this->debug_print_cache_set(addr, 0, 'w');
    //this->debug_print_prefetcher();
    // -------------------------------------------------------
}

#endif // CACHESIM_H
