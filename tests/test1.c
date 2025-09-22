int f(int a, int b) {
    int x;
    x = a;         
    int r = x + b; 
    r += x;        
    if (b > 0) {
        r += x;
    }
    return r;
}
