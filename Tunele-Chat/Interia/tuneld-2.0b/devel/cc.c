int czat2len(char *p) {
    int pre;
    pre = ((p[1]<<8)&0xffff) | (p[0]&0xff);

    return pre;
}

int main() {
char p[2];
p[0]=0x33;
p[1]=0x01;

printf("wynik: %d \n",czat2len(p));
}