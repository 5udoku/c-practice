#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

// Structura pentru BMP header
typedef struct {
    unsigned short type;        // Tipul fisierului, pentru BMP este 'BM'
    unsigned int size;          // Dimensiunea fisierului in octeti
    unsigned short reserved1;   // Camp rezervat (nu este folosit)
    unsigned short reserved2;   // Camp rezervat (nu este folosit)
    unsigned int offset;        // Offset-ul de la inceputul fisierului la inceputul bitmap-ului
    unsigned int dib_header_size; // Dimensiunea DIB header-ului
    int width;                  // Latimea imaginii
    int height;                 // Inaltimea imaginii
    // ... alte campuri care pot fi ignorate pentru acest program
} BMPHeader;

int main(int argc, char *argv[]) {
    // Verificam daca avem exact un argument (numele fisierului de intrare)
    if (argc != 2) {
        printf("Usage: %s <fisier_intrare>\n", argv[0]);
        return 1;
    }

    // Deschidem fisierul BMP pentru citire
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }

    BMPHeader bmp_header;
    // Citim BMP header-ul
    if (read(fd, &bmp_header, sizeof(BMPHeader)) != sizeof(BMPHeader)) {
        perror("Error reading BMP header");
        close(fd);
        return 1;
    }

    // Verificam daca fisierul este un BMP
    if (bmp_header.type != 0x4D42) { // 'BM' in hex
        printf("File is not a BMP image\n");
        close(fd);
        return 1;
    }

    // Informatii despre fisier folosind stat
    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        perror("Error getting file information");
        close(fd);
        return 1;
    }

    // Crearea fisierului de statistica
    int fd_stat = open("statistica.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_stat == -1) {
        perror("Error creating statistics file");
        close(fd);
        return 1;
    }

    char stat_buffer[1024];
    int bytes_written;

    // Scrierea in fisierul de statistica
    sprintf(stat_buffer, "nume fisier: %s\ninaltime: %d\nlungime: %d\ndimensiune: %ld\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %ld\ncontorul de legaturi: %ld\ndrepturi de acces user: %c%c%c\ndrepturi de acces grup: %c%c%c\ndrepturi de acces altii: %c%c%c\n",
            argv[1], bmp_header.height, bmp_header.width, file_stat.st_size, file_stat.st_uid,
            file_stat.st_mtime, file_stat.st_nlink,
            (file_stat.st_mode & S_IRUSR) ? 'R' : '-', (file_stat.st_mode & S_IWUSR) ? 'W' : '-', (file_stat.st_mode & S_IXUSR) ? 'X' : '-',
            (file_stat.st_mode & S_IRGRP) ? 'R' : '-', (file_stat.st_mode & S_IWGRP) ? '-' : '-', (file_stat.st_mode & S_IXGRP) ? '-' : '-',
            (file_stat.st_mode & S_IROTH) ? 'R' : '-', (file_stat.st_mode & S_IWOTH) ? '-' : '-', (file_stat.st_mode & S_IXOTH) ? '-' : '-');

    bytes_written = write(fd_stat, stat_buffer, strlen(stat_buffer));
    if (bytes_written == -1) {
        perror("Error writing to statistics file");
    }

    // Inchidem fisierele
    close(fd);
    close(fd_stat);

    return 0;
}

