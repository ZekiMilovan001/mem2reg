int casted(int a) {
    int x;
    void *p = &x;
    int *q = (int*)p;

    *q = a;  
    return *q; 
}
