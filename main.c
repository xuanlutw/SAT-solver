# include <stdio.h>
# include <stdlib.h>
# include <stdint.h>
# include <stdbool.h>
# include <string.h>

typedef uint32_t Literal;
# define Literal_bit            (8 * sizeof(Literal))
# define Literal_block(num_var) ((num_var / Literal_bit) + ((num_var % Literal_bit) > 0))

typedef struct Clause {
    uint16_t* num_var;
    Literal*  pos_lit;
    Literal*  neg_lit;
    uint8_t   var_count;
    int16_t   sat_assign;
    struct Clause* next;
    struct Clause* pre_s;
} Clause;

typedef struct {
    uint16_t num_var;
    uint16_t num_cla;
    Literal* cla_mem;
    Clause*  assign;
    Clause*  cla;
    Clause*  start;
    Clause*  top;
} Formula;

/* cla clause
 * lit literal
 * var variable */
# define NVAR(cla) (*(cla)->num_var)
# define set_lit_pos(cla, var_idx) \
    ( *((cla)->pos_lit + ((var_idx) - 1) / Literal_bit) |= (1 << ((var_idx) - 1) % Literal_bit))
# define set_lit_neg(cla, var_idx) \
    ( *((cla)->neg_lit + ((var_idx) - 1) / Literal_bit) |= (1 << ((var_idx) - 1) % Literal_bit))
# define set_lit(cla, var) \
    ((var) > 0? (set_lit_pos((cla), (var))): (set_lit_neg((cla), -(var))))

# define get_lit_pos(cla, var_idx) \
    ((*((cla)->pos_lit + ((var_idx) - 1) / Literal_bit) &  (1 << ((var_idx) - 1) % Literal_bit)) > 0)
# define get_lit_neg(cla, var_idx) \
    ((*((cla)->neg_lit + ((var_idx) - 1) / Literal_bit) &  (1 << ((var_idx) - 1) % Literal_bit)) > 0)
# define get_lit(cla, var_idx) \
    ((get_lit_pos((cla), (var_idx)) > 0)? 1: \
     ((get_lit_neg((cla), (var_idx)) > 0)? -1: 0))

# define check_lit(cla, var) \
    ((var) < 0? get_lit_neg((cla), -(var)) == 1: get_lit_pos((cla), (var)) == 1)
# define check_neg_lit(cla, var) \
    (check_lit((cla), -(var)))

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
    Formula* formula = malloc(sizeof(Formula));
    formula->num_var = num_var;
    formula->num_cla = num_cla;
    formula->assign  = malloc(sizeof(Clause));
    formula->cla     = malloc(num_cla * sizeof(Clause));
    formula->cla_mem = malloc((num_cla + 1) * 2 * Literal_block(num_var) * sizeof(Literal));
    memset(formula->cla_mem, 0, ((num_cla + 1) * 2 * Literal_block(num_var) * sizeof(Literal)));
    for (int i = 0; i < num_cla; ++i) {
        formula->cla[i].num_var    = &(formula->num_var);
        formula->cla[i].pos_lit    = formula->cla_mem + Literal_block(num_var) * (2 * i);
        formula->cla[i].neg_lit    = formula->cla_mem + Literal_block(num_var) * (2 * i + 1);
        formula->cla[i].var_count  = 0;
        formula->cla[i].sat_assign = 0;
        formula->cla[i].next       = NULL;
        formula->cla[i].pre_s      = NULL;
    }
    formula->assign->pos_lit = formula->cla_mem + Literal_block(num_var) * (2 * num_cla);
    formula->assign->neg_lit = formula->cla_mem + Literal_block(num_var) * (2 * num_cla + 1);
    formula->assign->num_var = &(formula->num_var);
    formula->start = NULL;
    formula->top   = NULL;
    return formula;
}

void print_clause(Clause* cla) {
    for (int i = NVAR(cla); i > 0; --i) {
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
    printf("=========================\n");
    for (int i = 0; i < formula->num_cla; ++i) {
        printf("%3d: ", i);
        print_clause(formula->cla + i);
    }
}

void print_formula2(Formula* formula) {
    Clause* cla = formula->start;
    printf("=========================\n");
    while (cla != NULL) {
        printf("XXX: ");
        print_clause(cla);
        cla = cla->next;
    }
}

bool clause_tauto(Clause* cla) {
    bool ret = false;
    for (int i = 0; i < Literal_block(NVAR(cla)); ++i)
        ret = ret || ((cla->pos_lit[i] & cla->neg_lit[i]) > 0);
    return ret;
}

Clause* clause_sort(Clause* cla) {
    // Base Case
    if (cla == NULL || cla->next == NULL)
        return cla;

    // Divid
    Clause* cla_half = cla;
    Clause* cla_tmp  = cla->next->next;
    Clause* cla_ret;
    while (cla_tmp != NULL && cla_tmp->next != NULL) {
        cla_half = cla_half->next;
        cla_tmp  = cla_tmp->next->next;
    }
    cla_tmp       = cla_half;
    cla_half      = cla_half->next;
    cla_tmp->next = NULL;
    cla           = clause_sort(cla);
    cla_half      = clause_sort(cla_half);

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

void push(Formula* formula, Clause* cla) {
    cla->pre_s   = formula->top;
    formula->top = cla;
}

Formula* load_cnf_file(const char* filename) {
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
    Clause*  tail    = NULL;
    for (int i = 0; i < num_cla; ++i) {
        int16_t tmp;
        while (true) {
            if ((tmp = get_int16(fp)) == 0)
                break;
            else
                set_lit(formula->cla + i, tmp);
                /*printf("%p\n", (cla_pos(formula, i) + ((-tmp - 1) / 8)));*/
            ++formula->cla[i].var_count;
        }
        /*printf(">>%p %p\n", cla_pos(formula, i), cla_neg(formula, i));*/
        /*printf("%d %d\n", *cla_pos(formula, i), *cla_neg(formula, i));*/
        if (clause_tauto(formula->cla + i))
            push(formula, formula->cla + i);
        else if (formula->start == NULL) {
            formula->start = formula->cla + i;
            tail = formula->cla + i;
        }
        else {
            tail->next = formula->cla + i;
            tail = formula->cla + i;
        }
    }
    print_formula(formula);
    print_formula2(formula);
    formula->start = clause_sort(formula->start);
    print_formula2(formula);

    return formula;
}

int16_t get_assign(const Clause* target, const Clause* assign, int16_t l_bound) {
    l_bound = l_bound < 0? -l_bound: l_bound;

    for (int i = 0; i < Literal_block(NVAR(target)); ++i) {
        Literal lit = (target->pos_lit[i] | target->neg_lit[i]) & \
                     ~(assign->pos_lit[i] | assign->neg_lit[i]);
        if (((i + 1) * Literal_bit < l_bound) || (lit == 0))
            continue;
        else
            while (lit != 0) {
                int idx = ffs(lit);
                if (idx + i * Literal_bit > l_bound)
                    return get_lit(target, idx + i * Literal_bit) * (idx + i * Literal_bit);
                lit &= ~(1 << (idx - 1));
            }
    }
    return 0;
}

void propagate(Formula* formula, int16_t var) {
    Clause  cla_head;
    Clause* cla_next;
    Clause* cla_now = &cla_head;
    cla_head.next = formula->start;
    while ((cla_next = cla_now->next) != NULL) {
        if (check_lit(cla_next, var)) {
            cla_now->next = cla_next->next;
            cla_next->sat_assign = var;
            cla_next->next       = NULL;
            push(formula, cla_next);
        }
        else if (check_neg_lit(cla_next, var)) {
            --cla_next->var_count;
            cla_now = cla_next;
        }
        else
            cla_now = cla_next;
    }
    formula->start = clause_sort(cla_head.next);
}

int main() {
    /*printf("%d\n", Literal_bit);*/
    Formula* formula = load_cnf_file("33.cnf");

    /*while (true) {*/
    for (int ccc = 0; ccc < 10; ++ccc) {
        if (formula->start == NULL) {
            printf("SAT\n");
            print_clause(formula->assign);
            break;
        }
        else if (formula->start->var_count > 0) {
            int16_t var = get_assign(formula->start, formula->assign, 0);
            set_lit(formula->assign, var);
            printf("Add var %d\n", var);
            propagate(formula, var);
            print_formula2(formula);
        }
        else {
            // BACK
        }
    }
    return 0;
}
