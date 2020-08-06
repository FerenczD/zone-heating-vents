#include "cmd_queue.h"

unsigned hash(uint8_t* s)
{
    unsigned hashval;
    int i = 0;
    for (hashval = 0; i < BLE_ADDR_LEN; s++,i++)
      hashval = *s + 31 * hashval;
    return hashval % HASHSIZE;
}

/* lookup: look for s in hashtab */
nlist* cmd_lookup(uint8_t* s)
{
    nlist *np;
    for (np = hashtab[hash(s)]; np != NULL; np = np->next){
      if (addr_cmp(s, np->ble_addr) == 0){
          return np; /* found */
      }
    }

    return NULL; /* not found */
}

int addr_cmp(uint8_t* s, uint8_t* target){

    for ( int i = 0; i < BLE_ADDR_LEN; i++, s++, target++){

        if(*s != *target){
            return 1;
        }
    }

    return 0;
}


/* install: put (name, defn) in hashtab */  
nlist* cmd_add(uint8_t* ble_addr, vent_cmd_t cmd)
{
    nlist* np;

    unsigned hashval;

    if ((np = cmd_lookup(ble_addr)) == NULL) { /* not found */

        np = ( nlist *) malloc(sizeof(*np));

        if (np == NULL){
          return NULL;
        }

        hashval = hash(ble_addr);
        np->next = hashtab[hashval];
        memcpy(np->ble_addr,ble_addr, BLE_ADDR_LEN);
        np->data = cmd;
        hashtab[hashval] = np;


    } else{

        memset(&np->data, 0, sizeof(np->data));; /*free previous cmd */
        np->data = cmd;
    } /* already there */

//    if (np->data == NULL){
//       return NULL;
//    }

    return np;
}

// free item
void free_cmd(nlist* np) {
    
    uint8_t addr_cpy[BLE_ADDR_LEN];
    memcpy(&addr_cpy, &np->ble_addr, BLE_ADDR_LEN);

    memset(&np->ble_addr, 0, sizeof(np->ble_addr));
    memset(&np->data, 0, sizeof(np->data));
    free(np);
    np = NULL;

//    free(hashtab[hash(&addr_cpy)]);
//    hashtab[hash(&addr_cpy)] = NULL;
}

void delete_cmd(nlist* cmd) {
    nlist* np = hashtab[hash(cmd->ble_addr)];

    while (np) {
        nlist* tmp = np;
        if (addr_cmp(tmp->ble_addr, cmd->ble_addr) == 0) {
            np = tmp->next;  // unlink the list node
            free_cmd(tmp);
            break;
        } else {
            np = np->next;
        }
    }
}