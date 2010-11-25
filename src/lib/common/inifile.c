/**
 * @file
 * @brief initialization file read and write API implementation
 * @author Deng Yangjun
 * @date 2007-1-9
 * @version 0.2
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "sys/sys.h"

#include "common\inifile.h"
#include "ff.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_FILE_SIZE 512
#define LEFT_BRACE '['
#define RIGHT_BRACE ']'

static int load_ini_file(const char *file, char *buf,int *file_size)
{
	FRESULT res;
	FIL file_obj;
	unsigned int br;
	int i = 0;

	assert(file !=NULL);
	assert(buf !=NULL);

	res = f_open(&file_obj, file, FA_OPEN_EXISTING | FA_READ);
	if( res != FR_OK) {
		return 0;
	}

	for (;;) {
		res = f_read(&file_obj,buf + i,1,&br);
		if (res || br == 0)
			break;
		i++;
		assert( i < MAX_FILE_SIZE ); //file too big, you can redefine MAX_FILE_SIZE to fit the big file 
	}

	buf[i]='\0';
	*file_size = i;

	f_close(&file_obj);
	return 1;
}

static int newline(char c)
{
	return ('\n' == c ||  '\r' == c )? 1 : 0;
}

static int end_of_string(char c)
{
	return '\0'==c? 1 : 0;
}

static int left_barce(char c)
{
	return LEFT_BRACE == c? 1 : 0;
}

static int isright_brace(char c )
{
	return RIGHT_BRACE == c? 1 : 0;
}

/*
 *[sector]         keyword       =valuevalue
 *|      |        |      |       |         |
 *sec_s  sec_e    key_s  key_e   value_s   value_e
 */
static int parse_file(const char *section, const char *key, const char *buf,int *sec_s,int *sec_e,
					  int *key_s,int *key_e, int *value_s, int *value_e)
{
	const char *p = buf;
	int i=0;

	assert(buf!=NULL);
	assert(section != NULL && strlen(section));
	assert(key != NULL && strlen(key));

	*sec_e = *sec_s = *key_e = *key_s = *value_s = *value_e = -1;

	while( !end_of_string(p[i]) ) {
		//find the section
		if( ( 0==i ||  newline(p[i-1]) ) && left_barce(p[i]) )
		{
			int section_start=i+1;

			//find the ']'
			do {
				i++;
			} while( !isright_brace(p[i]) && !end_of_string(p[i]));

			if( 0 == strncmp(p+section_start,section, i-section_start)) {
				int newline_start=0;

				i++;

				//Skip over space char after ']'
				while(isspace(p[i])) {
					i++;
				}

				//find the section
				*sec_s = section_start;
				*sec_e = i;

				while( ! (newline(p[i-1]) && left_barce(p[i])) 
				&& !end_of_string(p[i]) ) {
					int j=0;
					//get a new line
					newline_start = i;

					while( !newline(p[i]) &&  !end_of_string(p[i]) ) {
						i++;
					}
					
					//now i  is equal to end of the line
					j = newline_start;

					if(';' != p[j]) //skip over comment
					{
						while(j < i && p[j]!='=') {
							j++;
							if('=' == p[j]) {
								if(strncmp(key,p+newline_start,j-newline_start)==0)
								{
									//find the key ok
									*key_s = newline_start;
									*key_e = j-1;

									*value_s = j+1;
									*value_e = i;

									return 1;
								}
							}
						}
					}

					i++;
				}
			}
		}
		else
		{
			i++;
		}
	}
	return 0;
}

/**
*@brief read string in initialization file\n
* retrieves a string from the specified section in an initialization file
*@param section [in] name of the section containing the key name
*@param key [in] name of the key pairs to value 
*@param value [in] pointer to the buffer that receives the retrieved string
*@param size [in] size of result's buffer 
*@param default_value [in] default value of result
*@param file [in] path of the initialization file
*@return 1 : read success; \n 0 : read fail
*/
int read_profile_string( const char *section, const char *key,char *value, 
		 int size, const char *default_value, const char *file)
{
	char *buf;
	int file_size;
	int sec_s,sec_e,key_s,key_e, value_s, value_e;

	//check parameters
	assert(section != NULL && strlen(section));
	assert(key != NULL && strlen(key));
	assert(value != NULL);
	assert(size > 0);
	assert(file !=NULL &&strlen(key));

	buf = (char *)sys_malloc(MAX_FILE_SIZE);
	memset(buf,0,MAX_FILE_SIZE);

	if(!load_ini_file(file,buf,&file_size))
	{
		if(default_value!=NULL)
		{
			strncpy(value,default_value, size);
		}
		sys_free(buf);
		return 0;
	}

	if(!parse_file(section,key,buf,&sec_s,&sec_e,&key_s,&key_e,&value_s,&value_e))
	{
		if(default_value!=NULL)
		{
			strncpy(value,default_value, size);
		}
		sys_free(buf);
		return 0; //not find the key
	}
	else
	{
		int cpcount = value_e -value_s;

		if( size-1 < cpcount)
		{
			cpcount =  size-1;
		}
	
		memset(value, 0, size);
		memcpy(value,buf+value_s, cpcount );
		value[cpcount] = '\0';

		sys_free(buf);

		return 1;
	}
}

/**
*@brief read int value in initialization file\n
* retrieves int value from the specified section in an initialization file
*@param section [in] name of the section containing the key name
*@param key [in] name of the key pairs to value 
*@param default_value [in] default value of result
*@param file [in] path of the initialization file
*@return profile int value,if read fail, return default value
*/
int read_profile_int( const char *section, const char *key,int default_value, 
				const char *file)
{
	char value[32] = {0};
	if(!read_profile_string(section,key,value, sizeof(value),NULL,file))
	{
		return default_value;
	}
	else
	{
		return atoi(value);
	}
}

/**
 * @brief write a profile string to a ini file
 * @param section [in] name of the section,can't be NULL and empty string
 * @param key [in] name of the key pairs to value, can't be NULL and empty string
 * @param value [in] profile string value
 * @param file [in] path of ini file
 * @return 1 : success\n 0 : failure
 */
int write_profile_string(const char *section, const char *key,
					const char *value, const char *file)
{
	char *buf;
	char *w_buf;
	int sec_s,sec_e,key_s,key_e, value_s, value_e;
	int value_len = (int)strlen(value);
	int file_size;
	unsigned int new_file_size;

	//check parameters
	assert(section != NULL && strlen(section));
	assert(key != NULL && strlen(key));
	assert(value != NULL);
	assert(file !=NULL &&strlen(key));

	buf = (char *)sys_malloc(MAX_FILE_SIZE);
	w_buf = (char *)sys_malloc(MAX_FILE_SIZE);
	memset(buf,0,MAX_FILE_SIZE);
	memset(w_buf,0,MAX_FILE_SIZE);

	if(!load_ini_file(file,buf,&file_size))
	{
		sec_s = -1;
	}
	else
	{
		parse_file(section,key,buf,&sec_s,&sec_e,&key_s,&key_e,&value_s,&value_e);
	}

	if( -1 == sec_s)
	{
		if(0==file_size)
		{
			sprintf(w_buf,"[%s]\r\n%s=%s\r\n",section,key,value);
			new_file_size = strlen(section) + strlen(key) + strlen(value) + 5;
		}
		else
		{
			//not find the section, then add the new section at end of the file
			memcpy(w_buf,buf,file_size);
			sprintf(w_buf+file_size,"\r\n[%s]\r\n%s=%s\r\n",section,key,value);
			new_file_size = file_size + strlen(section) + strlen(key) + value_len + 7;
		}
	}
	else if(-1 == key_s)
	{
		//not find the key, then add the new key=value at end of the section
		memcpy(w_buf, buf, sec_e);
		sprintf(w_buf + sec_e, "%s=%s\r\n", key, value);
		memcpy(w_buf + sec_e + strlen(key) + value_len + 3,buf + sec_e, file_size - sec_e);
		new_file_size = file_size + strlen(key) + value_len + 3;
	}
	else
	{
		//update value with new value
		memcpy(w_buf, buf, value_s);
		memcpy(w_buf + value_s, value, value_len);
		memcpy(w_buf+value_s+value_len, buf+value_e, file_size - value_e);
		new_file_size = file_size - value_e + value_len + value_s;
	}

	FIL file_obj;
	unsigned int br;

	if (f_open(&file_obj, file, FA_WRITE)) {
		sys_free(buf);
		sys_free(w_buf);
		return 0;
	}

	if (f_write(&file_obj, w_buf, new_file_size, &br)) {
		sys_free(buf);
		sys_free(w_buf);
		return 0;
	}

	if (f_truncate(&file_obj)) {		/* Truncate unused area */
		sys_free(buf);
		sys_free(w_buf);
		return 0;
	}

	f_close(&file_obj);

	sys_free(buf);
	sys_free(w_buf);
	return 1;
}


#ifdef __cplusplus
}; //end of extern "C" {
#endif

#if 0
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>

static int cmd_ini_op(int argc, char *argv[])
{
	const char usage[] = { \
		" usage:\n" \
		" ini read sector key,read sector " \
	};
	
	if(argc < 4) {
		printf(usage);
		return 0;
	}
	
	char buffer[20];
	
	if (!strcmp(argv[1],"read")) {
		if (!read_profile_string(argv[2], argv[3], buffer, 20, "default_value", "test.ini"))
			printf("reaf failed!\r\n");
		else
			printf("%s\r\n",buffer);
	}
	
	if (!strcmp(argv[1],"write")) {
		if (!write_profile_string(argv[2], argv[3],  argv[4], "test.ini"))
			printf("reaf failed!\r\n");
	}

	return 0;
}
const cmd_t cmd_ini = {"ini", cmd_ini_op, "ini file operation"};
DECLARE_SHELL_CMD(cmd_ini)

#endif
