#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>

void handle_error(int status) {
    if (status != NC_NOERR) {
        fprintf(stderr, "Error: %s\n", nc_strerror(status));
        exit(1);
    }
}

int main( int argc, char *argv[]) {
    // Ruta al archivo NetCDF (actualiza esta ruta si es necesario)
    const char *file_path = "MERRA2_400.tavg1_2d_rad_Nx.20200115.nc4";
    
    printf("\n es: %s \n", argv[1]);
    
    //const char *file_path = argv[0];
    
    int ncid;  // Identificador del archivo NetCDF

    // Abrir el archivo en modo lectura
    int status = nc_open(file_path, NC_NOWRITE, &ncid);
    handle_error(status);

    // Variables para almacenar el número de dimensiones, variables y atributos
    int num_dims, num_vars, num_global_attrs, unlim_dim_id;

    // Obtener información general del archivo
    status = nc_inq(ncid, &num_dims, &num_vars, &num_global_attrs, &unlim_dim_id);
    handle_error(status);

    // Imprimir dimensiones
    printf("Dimensiones:\n");
    for (int i = 0; i < num_dims; i++) {
        char dim_name[NC_MAX_NAME + 1];
        size_t dim_length;
        status = nc_inq_dim(ncid, i, dim_name, &dim_length);
        handle_error(status);
        printf(" - %s: %zu\n", dim_name, dim_length);
    }

    // Imprimir variables
    printf("\nVariables:\n");
    for (int i = 0; i < num_vars; i++) {
        char var_name[NC_MAX_NAME + 1];
        int var_ndims, var_dimids[NC_MAX_DIMS], var_natts;
        nc_type var_type;

        // Obtener información de la variable
        status = nc_inq_var(ncid, i, var_name, &var_type, &var_ndims, var_dimids, &var_natts);
        handle_error(status);

        printf(" - %s (Tipo: %d, Dimensiones: %d): ", var_name, var_type, var_ndims);
        for (int j = 0; j < var_ndims; j++) {
            char dim_name[NC_MAX_NAME + 1];
            nc_inq_dimname(ncid, var_dimids[j], dim_name);
            printf("%s ", dim_name);
        }
        printf("\n");
    }

    // Imprimir atributos globales
    printf("\nAtributos Globales:\n");
    for (int i = 0; i < num_global_attrs; i++) {
        char attr_name[NC_MAX_NAME + 1];
        status = nc_inq_attname(ncid, NC_GLOBAL, i, attr_name);
        handle_error(status);

        // Solo imprimir el nombre del atributo
        printf(" - %s\n", attr_name);
    }

    // Cerrar el archivo NetCDF
    status = nc_close(ncid);
    handle_error(status);

    printf("\nArchivo NetCDF cerrado correctamente.\n");
    return 0;
}

