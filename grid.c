#include <stdio.h>

void display_grid(int size) {
    
    /* Print column numbers starting from 1 */
    printf("    ");
    for (int i = 1; i <= size; i++)
        printf("  %2d", i);
    printf("\n");

    /* Print second border */
    printf("    ");
    for (int i = 0; i < size; i++) {
        printf("+---");
    }
    printf("+\n");

    /* Print grid rows with row numbers starting from 1 */
    for (int i = 0; i < size; i++) {
        printf(" %2d ", i + 1);
        for (int j = 0; j < size; j++) {
            printf("| %c ", ' ');
        }
        printf("|\n");
        
        /* Print row separator */
        printf("    ");
        for (int j = 0; j < size; j++) {
            printf("+---");
        }
        printf("+\n");
    }
}

int main() {
    int size;
    printf("Enter grid size: ");
    scanf("%d", &size);

    display_grid(size);

    return 0;
}
