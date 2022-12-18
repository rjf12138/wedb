#include "sstable.h"

SStable::SStable()
:mem_db_(nullptr)
{
}

SStable::~SStable(void)
{
}

int 
SStable::write_to_file(MemoryDB *mem_db)
{

}

int 
SStable::read_from_file(MemoryDB *mem_db)
{

}

int 
SStable::create_data_block(void)
{
    if (mem_db_ == nullptr) {
        return -1;
    }

    basic::ByteBuffer data_block;
    basic::ByteBuffer filter_block;

    
}

int 
SStable::create_filter_block(void)
{

}

int 
SStable::create_meta_index_block(void)
{

}

int 
SStable::create_footer(void)
{

}