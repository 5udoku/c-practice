#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

typedef struct {
	unsigned short type;            // Tipul fisierului, pentru BMP este 'BM'
	unsigned int size;              // Dimensiunea fisierului in octeti
	unsigned short reserved1;       // Camp rezervat (nu este folosit)
	unsigned short reserved2;       // Camp rezervat (nu este folosit)
	unsigned int offset;            // Offset-ul de la inceputul fisierului la inceputul bitmap-ului
	unsigned int dib_header_size;   // Dimensiunea DIB header-ului
	int width;                      // Latimea imaginii
	int height;                     // Inaltimea imaginii
} BMPHeader;

void process_entry(const char *entry_name, const char* in_dir, const char* out_dir);
void write_file_info(int stat_file, struct stat *file_stat, const char *file_path, int is_bmp);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <director_intrare>\n", argv[0]);
        return 1;
    }

    DIR *inDir = opendir(argv[1]);
    if (inDir == NULL) {
        perror("Error opening input directory");
        return 1;
    }

    DIR *outDir = opendir(argv[2]);
    if (outDir == NULL) {
        perror("Error opening output directory");
        return 1;
    }

    struct dirent *entry;
    int nProc = 0;
    while ((entry = readdir(inDir)) != NULL) {
        // Ignore hidden files
        if(*entry->d_name == '.'){
            continue;
        }

        pid_t pid;
        if ((pid = fork()) < 0){
            perror("Error creating child process");
            exit(1);
        }
        if(pid == 0){
            // Child process
            process_entry(entry->d_name, argv[1], argv[2]);
            exit(0);
        }
        nProc++;

        if(strstr(entry->d_name,".bmp") != NULL){
            if((pid = fork()) < 0){
                perror("Error creating bmp child process");
                exit(1);
            }
            if(pid == 0){
                // Child process for BMP file
                
                exit(0);
            }
            nProc++;
        }
    }

    // Wait for child processes
    int status;
    for(int i = 0; i < nProc; i++){
        wait(&status);
        printf("> Child with pid %d exited. <\n", status);
    }

    closedir(inDir);
    closedir(outDir);
    return 0;
}

void process_entry(const char *entry_name, const char* in_dir, const char* out_dir) {
    // Dynamically create path to input entry
    int path_length = strlen(in_dir) + strlen(entry_name) + 2;
    char* inFile = (char*)malloc(path_length * sizeof(char));
    sprintf(inFile, "%s/%s", in_dir, entry_name);

    // Get input entry stats
    struct stat file_stat;
    if (lstat(inFile, &file_stat) == -1) {
        perror("Error getting file statistics");
        free(inFile);
        return;
    }

    // Get entry name without extension inside a buffer
    // const char* dot = strchr(entry_name, '.');
    // int buffer_size = dot - 
    // char buffer[32];
    // int buffer_size = 32;
    // if(dot - entry_name > 32){
    //     strncpy(buffer, entry_name, 32);
    // } else {
    //     strcpy(buffer, entry_name);
    // }
    
    // Create new statistics file
    path_length = strlen(out_dir) + strlen(entry_name) + 17;
    char* outFile = (char*)malloc(path_length * sizeof(char));
    sprintf(outFile, "%s/%s_statistica.txt", out_dir, entry_name);
    int stat_file = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (stat_file == -1) {
        perror("Error creating statistics file");
        free(inFile);
        free(outFile);
        return;
    }
    
    int is_bmp = 0;
    if(strstr(entry_name, ".bmp") != NULL){
        is_bmp = 1;
    }

    write_file_info(stat_file, &file_stat, entry_name, is_bmp);
    close(stat_file);
    free(inFile);
    free(outFile);
}

void write_file_info(int stat_file, struct stat *file_stat, const char *file_path, int is_bmp) {
    char stat_buffer[2048];
    int bytes_written;

    if (is_bmp) {
        BMPHeader bmp_header;
        int fd_bmp = open(file_path, O_RDONLY);
        if (fd_bmp == -1) {
            perror("Error opening BMP file");
            return;
        }
        if (read(fd_bmp, &bmp_header, sizeof(BMPHeader)) != sizeof(BMPHeader)) {
            perror("Error reading BMP header");
            close(fd_bmp);
            return;
        }
        close(fd_bmp);

        sprintf(stat_buffer, "nume fisier: %s\ninaltime: %d\nlungime: %d\ndimensiune: %lld\nidentificatorul utilizatorului: %d\ndrepturi de acces user: %c%c%c\ndrepturi de acces grup: %c%c%c\ndrepturi de acces altii: %c%c%c\n\n",
                file_path, bmp_header.height, bmp_header.width, file_stat->st_size, file_stat->st_uid,
                (file_stat->st_mode & S_IRUSR) ? 'R' : '-', (file_stat->st_mode & S_IWUSR) ? 'W' : '-', (file_stat->st_mode & S_IXUSR) ? 'X' : '-',
                (file_stat->st_mode & S_IRGRP) ? 'R' : '-', (file_stat->st_mode & S_IWGRP) ? 'W' : '-', (file_stat->st_mode & S_IXGRP) ? 'X' : '-',
                (file_stat->st_mode & S_IROTH) ? 'R' : '-', (file_stat->st_mode & S_IWOTH) ? 'W' : '-', (file_stat->st_mode & S_IXOTH) ? 'X' : '-');
    } else if (S_ISREG(file_stat->st_mode)) {
        // Pentru fisiere non-BMP
        sprintf(stat_buffer, "nume fisier: %s\ndimensiune: %lld\nidentificatorul utilizatorului: %d\ndrepturi de acces user: %c%c%c\ndrepturi de acces grup: %c%c%c\ndrepturi de acces altii: %c%c%c\n\n",
                file_path, file_stat->st_size, file_stat->st_uid,
                (file_stat->st_mode & S_IRUSR) ? 'R' : '-', (file_stat->st_mode & S_IWUSR) ? 'W' : '-', (file_stat->st_mode & S_IXUSR) ? 'X' : '-',
                (file_stat->st_mode & S_IRGRP) ? 'R' : '-', (file_stat->st_mode & S_IWGRP) ? 'W' : '-', (file_stat->st_mode & S_IXGRP) ? 'X' : '-',
                (file_stat->st_mode & S_IROTH) ? 'R' : '-', (file_stat->st_mode & S_IWOTH) ? 'W' : '-', (file_stat->st_mode & S_IXOTH) ? 'X' : '-');
    } else if (S_ISLNK(file_stat->st_mode)) {
        // Pentru legaturi simbolice
        struct stat target_stat;
        if (stat(file_path, &target_stat) == -1) {  // Get the stat of the target file
            perror("Error getting target file statistics");
            return;
        }

        sprintf(stat_buffer, "nume legatura: %s\ndimensiune legatura: %lld\ndimensiune fisier target: %lld\ndrepturi de acces user legatura: %c%c%c\ndrepturi de acces grup: %c%c%c\ndrepturi de acces altii: %c%c%c\n\n",
                file_path, file_stat->st_size, target_stat.st_size,
                (file_stat->st_mode & S_IRUSR) ? 'R' : '-', (file_stat->st_mode & S_IWUSR) ? 'W' : '-', (file_stat->st_mode & S_IXUSR) ? 'X' : '-',
                (file_stat->st_mode & S_IRGRP) ? 'R' : '-', (file_stat->st_mode & S_IWGRP) ? 'W' : '-', (file_stat->st_mode & S_IXGRP) ? 'X' : '-',
                (file_stat->st_mode & S_IROTH) ? 'R' : '-', (file_stat->st_mode & S_IWOTH) ? 'W' : '-', (file_stat->st_mode & S_IXOTH) ? 'X' : '-');
    } else if (S_ISDIR(file_stat->st_mode)) {
        // Pentru directoare
        sprintf(stat_buffer, "nume director: %s\nidentificatorul utilizatorului: %d\ndrepturi de acces user: %c%c%c\ndrepturi de acces grup: %c%c%c\ndrepturi de acces altii: %c%c%c\n\n",
                file_path, file_stat->st_uid,
                (file_stat->st_mode & S_IRUSR) ? 'R' : '-', (file_stat->st_mode & S_IWUSR) ? 'W' : '-', (file_stat->st_mode & S_IXUSR) ? 'X' : '-',
                (file_stat->st_mode & S_IRGRP) ? 'R' : '-', (file_stat->st_mode & S_IWGRP) ? 'W' : '-', (file_stat->st_mode & S_IXGRP) ? 'X' : '-',
                (file_stat->st_mode & S_IROTH) ? 'R' : '-', (file_stat->st_mode & S_IWOTH) ? 'W' : '-', (file_stat->st_mode & S_IXOTH) ? 'X' : '-');
    }

    bytes_written = write(stat_file, stat_buffer, strlen(stat_buffer));
    if (bytes_written == -1) {
        perror("Error writing to statistics file");
    }
}
