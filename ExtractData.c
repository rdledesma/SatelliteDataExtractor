#define _XOPEN_SOURCE 700  // Activa strptime en muchas configuraciones POSIX
#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <math.h>
#include <time.h>
void handle_error(int status) {
    if (status != NC_NOERR) {
        fprintf(stderr, "Error: %s\n", nc_strerror(status));
        exit(1);
    }
}

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

void process_file(const char *file_path, double target_lat, double target_lon, FILE *csv_file) {
    int ncid, time_varid, swtdn_varid, lat_varid, lon_varid;
    size_t time_len, lat_len, lon_len;
    char range_begin_date[20];

    // Abrir el archivo NetCDF
    int status = nc_open(file_path, NC_NOWRITE, &ncid);
    if (status != NC_NOERR) {
        fprintf(stderr, "Error al abrir el archivo %s: %s\n", file_path, nc_strerror(status));
        return;  // Continúa con el siguiente archivo
    }

    // Leer el atributo global RangeBeginningDate
    status = nc_get_att_text(ncid, NC_GLOBAL, "RangeBeginningDate", range_begin_date);
    handle_error(status);
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
    handle_error(status);
    status = nc_inq_dimlen(ncid, time_varid, &time_len);
    handle_error(status);

    status = nc_inq_varid(ncid, "lat", &lat_varid);
    handle_error(status);
    status = nc_inq_dimlen(ncid, lat_varid, &lat_len);
    handle_error(status);

    status = nc_inq_varid(ncid, "lon", &lon_varid);
    handle_error(status);
    status = nc_inq_dimlen(ncid, lon_varid, &lon_len);
    handle_error(status);

    status = nc_inq_varid(ncid, "SWTDN", &swtdn_varid);
    handle_error(status);

    // Leer los datos de latitudes y longitudes
    double *lat_data = (double *)malloc(lat_len * sizeof(double));
    double *lon_data = (double *)malloc(lon_len * sizeof(double));
    status = nc_get_var_double(ncid, lat_varid, lat_data);
    handle_error(status);
    status = nc_get_var_double(ncid, lon_varid, lon_data);
    handle_error(status);

    // Encontrar los índices de la latitud y longitud más cercanas
    int lat_index = find_nearest_index(lat_data, lat_len, target_lat);
    int lon_index = find_nearest_index(lon_data, lon_len, target_lon);

    // Leer los datos de tiempo y SWTDN para los índices de lat/lon específicos
    double *time_data = (double *)malloc(time_len * sizeof(double));
    status = nc_get_var_double(ncid, time_varid, time_data);
    handle_error(status);

    float *swtdn_data = (float *)malloc(time_len * sizeof(float));
    size_t start[] = {0, lat_index, lon_index};
    size_t count[] = {time_len, 1, 1};
    status = nc_get_vara_float(ncid, swtdn_varid, start, count, swtdn_data);
    handle_error(status);

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

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <directorio> <latitud> <longitud> <archivo_salida.csv>\n", argv[0]);
        return 1;
    }

    // Obtener parámetros de entrada
    const char *directory = argv[1];
    double target_lat = atof(argv[2]);
    double target_lon = atof(argv[3]);
    const char *output_csv = argv[4];

    // Abrir archivo CSV para escritura
    FILE *csv_file = fopen(output_csv, "w");
    if (!csv_file) {
        perror("Error al abrir el archivo CSV");
        return 1;
    }

    // Escribir encabezados en el CSV
    fprintf(csv_file, "datetime,SWTDN\n");

    // Abrir el directorio y procesar cada archivo .nc4
    DIR *dir = opendir(directory);
    if (!dir) {
        perror("Error al abrir el directorio");
        fclose(csv_file);
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Verificar si el archivo tiene extensión .nc4
        if (strstr(entry->d_name, ".nc4") != NULL) {
            char file_path[1024];
            snprintf(file_path, sizeof(file_path), "%s/%s", directory, entry->d_name);

            // Procesar el archivo .nc4
            process_file(file_path, target_lat, target_lon, csv_file);
        }
    }

    // Cerrar el directorio y el archivo CSV
    closedir(dir);
    fclose(csv_file);

    printf("Datos guardados en %s\n", output_csv);
    return 0;
}

