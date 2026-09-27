ASSERT(__heap_start + __heap_default_limit <= __stack - __stack_reserve_size, "test ROM heap overlaps the soft stack")
