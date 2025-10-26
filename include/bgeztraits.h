#ifndef BGEZTRAITS
#define BGEZTRAITS

using Trait = int;              //Traits of food. Each trait is identified by a particular bit in an integer.

enum class Traits : int{
    vegetarian    = 0x0001,
    vegan         = 0x0002,
    lowCarb       = 0x0004,
    keto          = 0x0008,
    trait5        = 0x0010,
    trait6        = 0x0020,
    trait7        = 0x0040,
    trait8        = 0x0080,
    trait9        = 0x0100,
    trait10       = 0x0200,
    trait11       = 0x0400,
    trait12       = 0x0800,
    trait13       = 0x1000,
    trait14       = 0x2000,
    trait15       = 0x4000,
    trait16       = 0x8000
};



#endif
