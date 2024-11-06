#define _XOPEN_SOURCE 700  // Activa strptime en muchas configuraciones POSIX
#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <math.h>
#include <time.h>

// Función para manejar errores de NetCDF
void handle_error(int status) {
    if (status != NC_NOERR) {
        fprintf(stderr, "Error: %s\n", nc_strerror(status));
        exit(1);
    }
}

// Función para encontrar el índice del valor más cercano en un array
int find_nearest_index(double *array, size_t len, double value) {
    int nearest_index = 0;
    double min_diff = fabs(array[0] - value);
    for (size_t i = 1; i < len; i++) {
        double diff = fabs(array[i] - value);
        if (diff < min_diff) {
            min_diff = diff;
            nearest_index = i;
        }
    }
    return nearest_index;
}

// Función para procesar cada archivo NetCDF
void process_file(const char *file_path, double target_lat, double target_lon, FILE *csv_file) {
    int ncid, time_varid, swtdn_varid, lat_varid, lon_varid;
    size_t time_len, lat_len, lon_len;
    char range_begin_date[20];

    // Intentar abrir el archivo NetCDF
    int status = nc_open(file_path, NC_NOWRITE, &ncid);
    if (status != NC_NOERR) {
        fprintf(stderr, "Error al abrir el archivo %s: %s\n", file_path, nc_strerror(status));
        return;  // Continúa con el siguiente archivo en caso de error
    }

    // Leer el atributo global RangeBeginningDate
    status = nc_get_att_text(ncid, NC_GLOBAL, "RangeBeginningDate", range_begin_date);
    if (status != NC_NOERR) {
        fprintf(stderr, "Error al leer el atributo RangeBeginningDate en el archivo %s: %s\n", file_path, nc_strerror(status));
        nc_close(ncid); // Cerrar archivo antes de continuar
        return;  // Continúa con el siguiente archivo
    }
    range_begin_date[10] = '\0';  // Asegurarse de que la cadena termine correctamente

    // Convertir RangeBeginningDate a una estructura de tiempo
    struct tm base_time = {0};
    strptime(range_begin_date, "%Y-%m-%d", &base_time);
    base_time.tm_hour = 0;
    base_time.tm_min = 0;
    base_time.tm_sec = 0;
    time_t base_timestamp = mktime(&base_time);

    // Obtener ID de las variables time, SWTDN, lat y lon
    status = nc_inq_varid(ncid, "time", &time_varid);
    if (status != NC_NOERR) {
        fprintf(stderr, "Error al obtener el ID de la variable 'time' en el archivo %s: %s\n", file_path, nc_strerror(status));
        nc_close(ncid);  // Cerrar archivo antes de continuar
        return;  // Continúa con el siguiente archivo
    }
    
    status = nc_inq_dimlen(ncid, time_varid, &time_len);
    if (status != NC_NOERR) {
        fprintf(stderr, "Error al obtener la longitud de la dimensión 'time' en el archivo %s: %s\n", file_path, nc_strerror(status));
        nc_close(ncid);  // Cerrar archivo antes de continuar
        return;  // Continúa con el siguiente archivo
    }

    status = nc_inq_varid(ncid, "lat", &lat_varid);
    if (status != NC_NOERR) {
        fprintf(stderr, "Error al obtener el ID de la variable 'lat' en el archivo %s: %s\n", file_path, nc_strerror(status));
        nc_close(ncid);  // Cerrar archivo antes de continuar
        return;  // Continúa con el siguiente archivo
    }
    
    status = nc_inq_dimlen(ncid, lat_varid, &lat_len);
    if (status != NC_NOERR) {
        fprintf(stderr, "Error al obtener la longitud de la dimensión 'lat' en el archivo %s: %s\n", file_path, nc_strerror(status));
        nc_close(ncid);  // Cerrar archivo antes de continuar
        return;  // Continúa con el siguiente archivo
    }

    status = nc_inq_varid(ncid, "lon", &lon_varid);
    if (status != NC_NOERR) {
        fprintf(stderr, "Error al obtener el ID de la variable 'lon' en el archivo %s: %s\n", file_path, nc_strerror(status));
        nc_close(ncid);  // Cerrar archivo antes de continuar
        return;  // Continúa con el siguiente archivo
    }
    
    status = nc_inq_dimlen(ncid, lon_varid, &lon_len);
    if (status != NC_NOERR) {
        fprintf(stderr, "Error al obtener la longitud de la dimensión 'lon' en el archivo %s: %s\n", file_path, nc_strerror(status));
        nc_close(ncid);  // Cerrar archivo antes de continuar
        return;  // Continúa con el siguiente archivo
    }

    status = nc_inq_varid(ncid, "SWTDN", &swtdn_varid);
    if (status != NC_NOERR) {
        fprintf(stderr, "Error al obtener el ID de la variable 'SWTDN' en el archivo %s: %s\n", file_path, nc_strerror(status));
        nc_close(ncid);  // Cerrar archivo antes de continuar
        return;  // Continúa con el siguiente archivo
    }

    // Leer los datos de latitudes y longitudes
    double *lat_data = (double *)malloc(lat_len * sizeof(double));
    double *lon_data = (double *)malloc(lon_len * sizeof(double));
    status = nc_get_var_double(ncid, lat_varid, lat_data);
    if (status != NC_NOERR) {
        fprintf(stderr, "Error al leer los datos de latitudes en el archivo %s: %s\n", file_path, nc_strerror(status));
        free(lat_data);
        free(lon_data);
        nc_close(ncid);  // Cerrar archivo antes de continuar
        return;  // Continúa con el siguiente archivo
    }

    status = nc_get_var_double(ncid, lon_varid, lon_data);
    if (status != NC_NOERR) {
        fprintf(stderr, "Error al leer los datos de longitudes en el archivo %s: %s\n", file_path, nc_strerror(status));
        free(lat_data);
        free(lon_data);
        nc_close(ncid);  // Cerrar archivo antes de continuar
        return;  // Continúa con el siguiente archivo
    }

    // Encontrar los índices de la latitud y longitud más cercanas
    int lat_index = find_nearest_index(lat_data, lat_len, target_lat);
    int lon_index = find_nearest_index(lon_data, lon_len, target_lon);

    // Leer los datos de tiempo y SWTDN para los índices de lat/lon específicos
    double *time_data = (double *)malloc(time_len * sizeof(double));
    status = nc_get_var_double(ncid, time_varid, time_data);
    if (status != NC_NOERR) {
        fprintf(stderr, "Error al leer los datos de tiempo en el archivo %s: %s\n", file_path, nc_strerror(status));
        free(lat_data);
        free(lon_data);
        free(time_data);
        nc_close(ncid);  // Cerrar archivo antes de continuar
        return;  // Continúa con el siguiente archivo
    }

    float *swtdn_data = (float *)malloc(time_len * sizeof(float));
    size_t start[] = {0, lat_index, lon_index};
    size_t count[] = {time_len, 1, 1};
    status = nc_get_vara_float(ncid, swtdn_varid, start, count, swtdn_data);
    if (status != NC_NOERR) {
        fprintf(stderr, "Error al leer los datos de SWTDN en el archivo %s: %s\n", file_path, nc_strerror(status));
        free(lat_data);
        free(lon_data);
        free(time_data);
        free(swtdn_data);
        nc_close(ncid);  // Cerrar archivo antes de continuar
        return;  // Continúa con el siguiente archivo
    }

    // Escribir los datos en el archivo CSV
    for (size_t t = 0; t < time_len; t++) {
        // Calcular datetime sumando minutos a base_timestamp
        time_t datetime_timestamp = base_timestamp + (time_t)(time_data[t] * 60);
        struct tm *datetime = localtime(&datetime_timestamp);

        // Formatear el datetime a string
        char datetime_str[20];
        strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M", datetime);

        // Escribir en el CSV
        fprintf(csv_file, "%s,%f\n", datetime_str, swtdn_data[t]);
    }

    // Liberar memoria y cerrar archivo NetCDF
    free(lat_data);
    free(lon_data);
    free(time_data);
    free(swtdn_data);
    nc_close(ncid);
}

// Función principal
int main() {
    // Definir las coordenadas de latitud y longitud objetivo
    double target_lat = 40.0;  // Ejemplo de latitud
    double target_lon = -75.0; // Ejemplo de longitud

    // Abrir archivo CSV de salida
    FILE *csv_file = fopen("output.csv", "w");
    if (!csv_file) {
        perror("No se puede abrir el archivo CSV");
        return 1;
    }

    // Escribir el encabezado del archivo CSV
    fprintf(csv_file, "DateTime,SWTDN\n");

    // Directorio que contiene los archivos NetCDF
    const char *dir_path = "Data/";

    // Abrir el directorio
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("No se puede abrir el directorio");
        fclose(csv_file);
        return 1;
    }

    // Leer los archivos del directorio
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Solo procesar archivos con extensión .nc4
        if (strstr(entry->d_name, ".nc4")) {
            char file_path[256];
            snprintf(file_path, sizeof(file_path), "%s%s", dir_path, entry->d_name);
            process_file(file_path, target_lat, target_lon, csv_file);
        }
    }

    // Cerrar el directorio y el archivo CSV
    closedir(dir);
    fclose(csv_file);

    return 0;
}

