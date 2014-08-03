If to set SIZE_ALLOC to 64, 80, 96 the program will fail the first-fit test, 
since c[19] will be allocated just below the allocated block, 
so after freeing c[19] there will be only 16 kBytes 
(that's why 56 kBytes have been chosen)
