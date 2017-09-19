extern "C" {
    #include <libcgc.h>
    #include <stdlib.h>
};

extern void terminate(int);

extern "C" void __cxa_pure_virtual()
{
    printf("Virtual Function....\n");
}

void *operator new( size_t size )
{
    return malloc(size);
}

void *operator new[]( size_t size )
{
    return ::operator new(size);
}

void operator delete(void *p)
{
    free(p);
}

void operator delete[](void *p)
{
    ::operator delete(p);
}

