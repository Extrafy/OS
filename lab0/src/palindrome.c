#include <stdio.h>
char s1[10005],s2[10005];
int cnt1;
int main() {
	int n;
	scanf("%d", &n);
	int flag = 1;
	while(n){
		int a = n%10;
		s1[cnt1++] = a+'0';
		n/=10;
	}
	for(int i=cnt1-1,j=0;i>=0;i--,j++){
		s2[j]=s1[i];
	}
	for(int i=0;i<cnt1;i++){
		if(s1[i]!=s2[i]){
			flag=0;
			break;
		}
	}
	if (flag) {
		printf("Y\n");
	} else {
		printf("N\n");
	}
	return 0;
}
