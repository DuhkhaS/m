#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h> //for getpagesize()
#include<errno.h>
#include<stdint.h>
#include<assert.h>

#define PAGEMAP_ENTRY 8
#define GET_BIT(X,Y) (X & ((uint64_t)1<<Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF

char path_buf [0x100] = {};
int start_pfn[3];
uint64_t read_val, file_offset;
int read_zoneinfo(){
	FILE *f;
	char temp[200];
	int i = 0;
	char *path_zone = "/proc/zoneinfo";
	f = fopen(path_zone,"r");
	while(!feof(f)){
		fgets(temp,1000,f);
		if(strstr(temp,"start_pfn")){	
			sscanf(temp," start_pfn: %d",&start_pfn[i++]);
		}
	}
	fclose(f);
}

int read_pagemap(char *path_buf,unsigned long virt_addr){
	int c;
	int status;
	FILE *f;
	f = fopen(path_buf,"rb");
	if(!f){
		printf("Can't open this path : %s\n",path_buf);
		return -1;
	}
	file_offset = virt_addr/getpagesize() * PAGEMAP_ENTRY;
	printf("Vaddr: 0x%lx, Page_size: %d, Entry_size: %d\n", virt_addr, getpagesize(), PAGEMAP_ENTRY);
	printf("Reading %s at 0x%llx\n", path_buf, (unsigned long long) file_offset);
	status = fseek(f, file_offset, SEEK_SET);
	if(status){
      		perror("Failed to do fseek!");
      		return -1;
   	}
	errno = 0;
   	read_val = 0;
	unsigned char c_buf[PAGEMAP_ENTRY];
	for(int i=0; i < PAGEMAP_ENTRY; i++){
      	c = getc(f);
      	if(c==EOF){
         	printf("\nReached end of the file\n");
         	return 0;
      	}
      	else
           c_buf[PAGEMAP_ENTRY - i - 1] = c;
      	printf("[%d]0x%x ", i, c);
	}
   	for(int i=0; i < PAGEMAP_ENTRY; i++){
      //printf("%d ",c_buf[i]);
      read_val = (read_val << 8) + c_buf[i];
   	}
   	printf("\n");
   	printf("Result: 0x%llx\n", (unsigned long long) read_val);
   	//if(GET_BIT(read_val, 63))
   	if(GET_BIT(read_val, 63)){
		printf("PFN: 0x%llx\n",(unsigned long long) GET_PFN(read_val));
		unsigned long long int phy = (unsigned long long)GET_PFN(read_val);	
		
		if(phy<start_pfn[1]){
			printf("DMA\n");		
		}
		else if(phy<start_pfn[2]){
			printf("DMA32\n");
		}
		else{
			printf("Normal\n");	
		}
	}
   	else
    	printf("Page not present\n");
   	if(GET_BIT(read_val, 62))
      	printf("Page swapped\n");
	fclose(f);
   	return 0;
}
void to_physical(char *pid,unsigned long *virt_addr,int n,int *size){
	//printf("%d\n",n);
	sprintf(path_buf, "/proc/%s/pagemap", pid);
	
	for(int i = 0;i < n;i++){
		read_pagemap(path_buf,virt_addr[i]);	
		printf("Size: %d\n",size[i]);
		printf("-----------------------------\n");
	}
}

void mem_use(char *pids){
	char *pre = "/proc/";
	char *suf = "/smaps";
	FILE *f;
	//char data[99999];
	unsigned long start[2048];
	unsigned long end;
	int c = 0;
	int flag = 0;
	char str[1024];
	int size[2048];
	char unit[10];
	int i;
	int n; //n = return no of data from maps
	char *status = (char *)malloc(strlen(pids) +strlen(pre) + strlen(suf)+1);
	sprintf(status,"%s%s%s",pre,pids,suf);	
	f = fopen(status,"r");
	if(f == NULL){
		
		printf("Cannot access this path or you must be root\n");
	}
	else{
		flag = 1;
		i = 1;
		while(fgets(str,1024,f)!=NULL){
			//strcat(data,str);
			if(i==1)
				n = sscanf(str,"%lX-%*s",&start[c]);
			else if(i==2)
				n = sscanf(str,"Size: %d %s",&size[c++],unit);
			i++;
			if(i>21){
				i=1;			
			}
			//printf("%lX \n",start[c++]);
		}	
		//printf("%s",data);
		fclose(f);
	}	
	printf("%s\n",status);		
	free(status);
	if(flag)
		to_physical(pids,start,c,size);
	exit(0);
}

int main(int argc,char **argv)
{
	if (argc == 1){
		printf("Please insert pid(s)\n");
	}
	else if(argc > 2){
		printf("Can't print more than 1 pid\n");	
	}
	else{
		read_zoneinfo();
		mem_use(argv[1]);
		
	}
	return 0;
}
