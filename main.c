# include <stdio.h>
# include <stdlib.h>
# include <stdint.h>
# include <stdbool.h>
# include <string.h>

typedef uint64_t Literal;
# define Literal_bit (8 * sizeof(Literal))

typedef struct Clause {
    uint16_t* num_var;
    Literal*  pos_lit;
    Literal*  neg_lit;
    uint8_t   var_count;
    struct Clause* next;
    struct Clause* pre_s;
} Clause;

typedef struct {
    uint16_t num_var;
    uint16_t num_cla;
    Literal* cla_mem;
    Clause*  cla;
    Clause*  start;
    Clause*  top;
} Formula;

/* cla clause
 * lit literal
 * var variable */
# define set_lit_pos(clause, num_var) \
    ( *((clause)->pos_lit + ((num_var) - 1) / Literal_bit) |= (1 << ((num_var) - 1) % Literal_bit))
# define set_lit_neg(clause, num_var) \
    ( *((clause)->neg_lit + ((num_var) - 1) / Literal_bit) |= (1 << ((num_var) - 1) % Literal_bit))

# define get_lit_pos(clause, num_var) \
    ((*((clause)->pos_lit + ((num_var) - 1) / Literal_bit) &  (1 << ((num_var) - 1) % Literal_bit)) > 0)
# define get_lit_neg(clause, num_var) \
    ((*((clause)->neg_lit + ((num_var) - 1) / Literal_bit) &  (1 << ((num_var) - 1) % Literal_bit)) > 0)
# define get_lit(clause, num_var) \
    ((get_lit_pos((clause), (num_var)) > 0)? 1: \
     ((get_lit_neg((clause), (num_var)) > 0)? -1: 0))

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
    const uint16_t num_uint8_per_lit = ((num_var / Literal_bit) + ((num_var % Literal_bit) > 0));
    Formula* formula = malloc(sizeof(Formula));
    formula->num_var = num_var;
    formula->num_cla = num_cla;
    formula->cla_mem = malloc(num_cla * 2 * num_uint8_per_lit * Literal_bit);
    formula->cla     = malloc(num_cla * sizeof(Clause));
    memset(formula->cla, 0, (num_cla * 2 * num_uint8_per_lit * Literal_bit));
    for (int i = 0; i < num_cla; ++i) {
        formula->cla[i].pos_lit   = formula->cla_mem + num_uint8_per_lit * (2 * i);
        formula->cla[i].neg_lit   = formula->cla_mem + num_uint8_per_lit * (2 * i + 1);
        formula->cla[i].var_count = 0;
        formula->cla[i].next      = formula->cla + (i + 1);
        formula->cla[i].pre_s     = NULL;
    }
    formula->cla[num_cla - 1].next = NULL;
    formula->start = formula->cla;
    formula->top   = NULL;
    return formula;
}

void print_clause(Clause* cla, uint16_t num_var) {
    for (int i = num_var; i > 0; --i) {
        switch (get_lit(cla, i)) {
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
    printf(" %3d\n", cla->var_count);
}

void print_formula(Formula* formula) {
    for (int i = 0; i < formula->num_cla; ++i) {
        printf("%3d: ", i);
        print_clause(formula->cla + i, formula->num_var);
    }
}

void print_formula2(Formula* formula) {
    Clause* cla = formula->start;
    while (cla) {
        printf("XXX: ");
        print_clause(cla, formula->num_var);
        cla = cla->next;
    }
}

Clause* clause_sort(Clause* cla, uint16_t num) {
    uint16_t num_half = num / 2;
    Clause* cla_half = cla;
    Clause* cla_tmp;
    Clause* cla_ret;
    if (num == 1)
        return cla;
    // Divid
    for (int i = 0; i < num_half - 1; ++i)
        cla_half = cla_half->next;
    cla_tmp       = cla_half;
    cla_half      = cla_half->next;
    cla_tmp->next = NULL;
    cla           = clause_sort(cla, num_half);
    cla_half      = clause_sort(cla_half, num - num_half);

    if (cla->var_count <= cla_half->var_count) {
        cla_ret = cla;
        cla_tmp = cla;
        cla     = cla->next;
    }
    else {
        cla_ret  = cla_half;
        cla_tmp  = cla_half;
        cla_half = cla_half->next;
    }

    while (true) {
        if (cla == NULL && cla_half == NULL)
            break;
        else if (cla == NULL) {
            cla_tmp->next = cla_half;
            cla_half = cla_half->next;
        }
        else if (cla_half == NULL) {
            cla_tmp->next = cla;
            cla = cla->next;
        }
        else if (cla->var_count <= cla_half->var_count) {
            cla_tmp->next = cla;
            cla = cla->next;
        }
        else {
            cla_tmp->next = cla_half;
            cla_half = cla_half->next;
        }
        cla_tmp = cla_tmp->next;
        cla_tmp->next = NULL;
    }

    return cla_ret;
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
                set_lit_pos(formula->cla + i, tmp);
            }
            else
                /*printf("%p\n", (cla_pos(formula, i) + ((-tmp - 1) / 8)));*/
                set_lit_neg(formula->cla + i, (-tmp));
            ++formula->cla[i].var_count;
        }
        /*printf(">>%p %p\n", cla_pos(formula, i), cla_neg(formula, i));*/
        /*printf("%d %d\n", *cla_pos(formula, i), *cla_neg(formula, i));*/
    }
    print_formula(formula);
    print_formula2(formula);
    formula->start = clause_sort(formula->start, num_var);
    print_formula2(formula);
}

int main() {
    printf("%d\n", Literal_bit);
    load_cnf_file("33.cnf");
    return 0;
}
