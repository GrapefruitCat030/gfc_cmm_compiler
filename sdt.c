#include "sdt.h"

#include "syntax.tab.h"

#include "util.h"

#include "tree.h"

#include "semantic.h"
#include "ir-gen.h"



void init_sdt() {
    init_semantic();
    init_irgen();
}

void boot_sdt(struct node *root) {
    boot_semantic(root);
    boot_irgen(root);
}


