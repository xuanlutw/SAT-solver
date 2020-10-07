# include <stdio.h>
# include <stdlib.h>
# include <stdint.h>
# include <stdbool.h>
# include <string.h>

/*typedef struct Clause {*/
    /*uint8_t* pos_lit;*/
    /*uint8_t* neg_lit;*/
    /*uint8_t  var_count;*/
    /*struct Clause* next;*/
/*} Clause;*/

typedef struct {
    uint16_t num_var;
    uint16_t num_cla;
    uint16_t num_uint8_lit;
    uint8_t* cla;
    /*Clause*  cla;*/
} Formula;

/* cla clause
 * lit literal
 * var variable */
# define cla_pos(formula, num_cla) \
    ((formula)->cla + ((formula)->num_uint8_lit) * (2 * (num_cla)))
# define cla_neg(formula, num_cla) \
    ((formula)->cla + ((formula)->num_uint8_lit) * (2 * (num_cla) + 1))
# define set_lit_pos(formula, num_cla, num_var) \
    ( *(cla_pos((formula), (num_cla)) + ((num_var) / 8)) |= (1 << ((num_var) % 8)))
# define set_lit_neg(formula, num_cla, num_var) \
    ( *(cla_neg((formula), (num_cla)) + ((num_var) / 8)) |= (1 << ((num_var) % 8)))
# define get_lit_pos(formula, num_cla, num_var) \
    ((*(cla_pos((formula), (num_cla)) + ((num_var) / 8)) &  (1 << ((num_var) % 8))) > 0)
# define get_lit_neg(formula, num_cla, num_var) \
    ((*(cla_neg((formula), (num_cla)) + ((num_var) / 8)) &  (1 << ((num_var) % 8))) > 0)
# define get_lit(formula, num_cla, num_var) \
    ((get_lit_pos((formula), (num_cla), (num_var)) > 0)? 1: \
     ((get_lit_neg((formula), (num_cla), (num_var)) > 0)? -1: 0))

void exiterr(const char* msg) {
    printf("%s", msg);
    exit(-1);
}

int16_t get_int16(FILE* fp) {
    int c;
    bool fl   = false;
    bool sign = true;
    int16_t ans = 0;
    while ((c = fgetc(fp)) != EOF) {
        if ((char)c == '-') {
            sign = false;
        }
        else if ((char)c >= '0' && (char)c <= '9') {
            fl = true;
            ans = ans * 10 + (c - '0');
        }
        else if (fl)
            break;
    }
    return (sign)? ans: -ans;
}

Formula* init_formula(uint16_t num_var, uint16_t num_cla) {
    Formula* formula       = malloc(sizeof(Formula));
    formula->num_var       = num_var;
    formula->num_cla       = num_cla;
    formula->num_uint8_lit = ((num_var / 8) + ((num_var % 8) > 0));
    formula->cla           = malloc(num_cla * 2 * formula->num_uint8_lit * \
                                    sizeof(uint8_t));
    memset(formula->cla, 0, (num_cla * 2 * formula->num_uint8_lit * \
                             sizeof(uint8_t)));
    /*formula->cla     = malloc(num_cla * sizeof(Clause));*/
    return formula;
}

void print_clause(Formula* formula, uint16_t num_cla) {
    printf("%3d: ", num_cla);
    for (int i = formula->num_var - 1; i >= 0; --i) {
        switch (get_lit(formula, num_cla, i)) {
            case 1:
                printf("T");
                break;
            case 0:
                printf("-");
                break;
            case -1:
                printf("F");
                }
    }
    printf("\n");
}

void print_formula(Formula* formula) {
    for (int i = 0; i < formula->num_cla; ++i)
        print_clause(formula, i);
}

void load_cnf_file(const char* filename) {
    int c;
    FILE* fp = fopen(filename, "r");

    uint16_t num_var = 0;
    uint16_t num_cla = 0;

    while ((c = fgetc(fp)) != EOF) {
        if ((char) c == 'c')
            while (((c = fgetc(fp)) != EOF) && ((char)c != '\n'));
        if ((char) c == 'p') {
            if ((char) fgetc(fp) != ' ') exiterr("Wrong format.\n");
            if ((char) fgetc(fp) != 'c') exiterr("Wrong format.\n");
            if ((char) fgetc(fp) != 'n') exiterr("Wrong format.\n");
            if ((char) fgetc(fp) != 'f') exiterr("Wrong format.\n");
            num_var = (uint16_t)get_int16(fp);
            num_cla = (uint16_t)get_int16(fp);
            break;
        }
    }

    Formula* formula = init_formula(num_var, num_cla);
    for (int i = 0; i < num_cla; ++i) {
        int16_t tmp;
        while (true) {
            tmp = get_int16(fp);
            if (tmp == 0)
                break;
            else if (tmp > 0) {
                set_lit_pos(formula, i, tmp - 1);
            }
            else
                /*printf("%p\n", (cla_pos(formula, i) + ((-tmp - 1) / 8)));*/
                set_lit_neg(formula, i, (-tmp) - 1);
        }
        /*printf(">>%p %p\n", cla_pos(formula, i), cla_neg(formula, i));*/
        /*printf("%d %d\n", *cla_pos(formula, i), *cla_neg(formula, i));*/
    }
    print_formula(formula);
}

int main() {
    load_cnf_file("33.cnf");
    return 0;
}
