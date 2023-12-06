#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

typedef struct {
	unsigned short type;        // Tipul fisierului, pentru BMP este 'BM'
	unsigned int size;          // Dimensiunea fisierului in octeti
	unsigned short reserved1;   // Camp rezervat (nu este folosit)
	unsigned short reserved2;   // Camp rezervat (nu este folosit)
	unsigned int offset;        // Offset-ul de la inceputul fisierului la inceputul bitmap-ului
	unsigned int dib_header_size; // Dimensiunea DIB header-ului
	int width;                  // Latimea imaginii
	int height;                 // Inaltimea imaginii
} BMPHeader;

void process_file(const char *file_path, int fd_stat);
void write_file_info(int fd_stat, struct stat *file_stat, const char *file_path, int is_bmp);
void handle_file(char* d_name, char* inDirPath, char* outDirPath);

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
        pid_t pid, pid_bmp;
        if ((pid = fork()) < 0){
            perror("Error creating child process");
            exit(1);
        }
        if( pid == 0){
            handle_file(entry->d_name, argv[1], argv[2]);
            exit(0);
        }
        nProc++;

        if(strstr(entry->d_name,".bmp") != NULL){
            if((pid = fork()) < 0){
                perror("Error creating bmp child process");
                exit(1);
            }
            if(pid == 0){
                // handle_bmp
                exit(0);
            }
            nProc++;
        }
    }

    int status;
    for(int i = 0; i < nProc; i++){
        wait(&status);
        printf("Child with pid %d exited.", status);
    }

    closedir(inDir);
    closedir(outDir);
    return 0;
}

void process_file(const char *file_path, int fd_stat) {
    struct stat file_stat;
    if (stat(file_path, &file_stat) == -1) {
        perror("Error getting file statistics");
        return;
    }

    int is_bmp = 0;
    const char *ext = strrchr(file_path, '.');
    if (ext != NULL && strcmp(ext, ".bmp") == 0) {
        is_bmp = 1;
    }

    write_file_info(fd_stat, &file_stat, file_path, is_bmp);
}

void write_file_info(int fd_stat, struct stat *file_stat, const char *file_path, int is_bmp) {
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

        sprintf(stat_buffer, "nume fisier: %s\ninaltime: %d\nlungime: %d\ndimensiune: %ld\nidentificatorul utilizatorului: %d\ndrepturi de acces user: %c%c%c\ndrepturi de acces grup: %c%c%c\ndrepturi de acces altii: %c%c%c\n\n",
                file_path, bmp_header.height, bmp_header.width, file_stat->st_size, file_stat->st_uid,
                (file_stat->st_mode & S_IRUSR) ? 'R' : '-', (file_stat->st_mode & S_IWUSR) ? 'W' : '-', (file_stat->st_mode & S_IXUSR) ? 'X' : '-',
                (file_stat->st_mode & S_IRGRP) ? 'R' : '-', (file_stat->st_mode & S_IWGRP) ? 'W' : '-', (file_stat->st_mode & S_IXGRP) ? 'X' : '-',
                (file_stat->st_mode & S_IROTH) ? 'R' : '-', (file_stat->st_mode & S_IWOTH) ? 'W' : '-', (file_stat->st_mode & S_IXOTH) ? 'X' : '-');
    } else if (S_ISREG(file_stat->st_mode)) {
        // Pentru fisiere non-BMP
        sprintf(stat_buffer, "nume fisier: %s\ndimensiune: %ld\nidentificatorul utilizatorului: %d\ndrepturi de acces user: %c%c%c\ndrepturi de acces grup: %c%c%c\ndrepturi de acces altii: %c%c%c\n\n",
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

        sprintf(stat_buffer, "nume legatura: %s\ndimensiune legatura: %ld\ndimensiune fisier target: %ld\ndrepturi de acces user legatura: %c%c%c\ndrepturi de acces grup: %c%c%c\ndrepturi de acces altii: %c%c%c\n\n",
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

    bytes_written = write(fd_stat, stat_buffer, strlen(stat_buffer));
    if (bytes_written == -1) {
        perror("Error writing to statistics file");
    }
}

void handle_file(char* d_name, char* inDirPath, char* outDirPath){
    char* outFile = NULL;
    sprintf(outFile, "%s/%s_statistica.txt", outDirPath, d_name);
    int fd_stat = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_stat == -1) {
        perror("Error creating statistics file");
        return;
    }

    char* inFile = NULL;
    sprintf(inFile, "%s/%s", inDirPath, d_name);
    struct stat file_stat;
    if (lstat(inFile, &file_stat) == -1) {
        perror("Error getting file statistics");
        close(fd_stat);
    }

    process_file(inFile, fd_stat);
    close(fd_stat);
}