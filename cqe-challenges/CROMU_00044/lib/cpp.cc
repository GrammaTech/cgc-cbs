
extern "C"
{
#include <libcgc.h>
#include <malloc.h>
}

void *operator new( size_t alloc_size )
{
    return (void *)malloc( alloc_size );
}

void *operator new[]( size_t alloc_size )
{
    return (void *)malloc( alloc_size );
}

void operator delete( void *ptr )
{
    free( ptr );
}

void operator delete[]( void *ptr )
{
    free( ptr );
}
