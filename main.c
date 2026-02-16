#include "Library.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>


int SLOTS;
library_t lib;
/* --- Phase 2 Helper for free_all --- */

/* forward declarations for helper/search functions (defined later) */
bool find_genre_gid(library_t *lib, int gid, genre_t **out_genre);
bool find_member_sid(library_t *lib, int sid, member_t **out_member);
bool find_book_bid_genre(genre_t *genre, int bid, book_t **out_book);
bool find_loan_bid_member(member_t *member, int bid, loan_t **out_loan);
bool find_book_bid_all_genres(library_t *lib, int bid, book_t **out_book);
/* === PHASE 2 DECLARATIONS === */

/* BookIndex (AVL) Helpers */
book_index_t* avl_insert(book_index_t *node, book_t *book);
book_index_t* avl_delete(book_index_t *root, const char *title);
book_index_t* avl_search(book_index_t *root, const char *title);

/* Recommendation Heap Helpers */
recommendation_heap_t* init_recommendation_heap(int capacity);
void heap_insert(recommendation_heap_t *h, book_t *book);
void heap_update_key(recommendation_heap_t *h, int i);
book_t* heap_remove_by_pos(recommendation_heap_t *h, int i);

/* Member Activity Helpers */
member_activity_t* find_or_create_activity(library_t *lib, int sid, const char *name);

/* === END PHASE 2 FORWARD DECLARATIONS === */

// 
void free_book_index(book_index_t *root) {
    if (root == NULL) {
        return;
    }
    free_book_index(root->lc); 
    free_book_index(root->rc); 
    free(root); // 
}

// 
void free_recommendation_heap(recommendation_heap_t *heap) {
    if (heap != NULL) {
        free(heap->heap); 
        free(heap);        
    }
}

void free_all(library_t* lib){
// Free olas ta members kai t loans tous 
    member_t *m = lib->members;
    while (m != NULL) {
        loan_t *loan = m->loans;
        while (loan != NULL) {
            loan_t *next_loan = loan->next;
            free(loan);
            loan = next_loan;
        }
        member_t *next_m = m->next;
        free(m);
        m = next_m;
    }

    // Free ola ta genres kai t books tous
    genre_t *g = lib->genres;
    while (g != NULL) {
        book_t *b = g->books;
        while (b != NULL) {
            book_t *next_b = b->next;
            free(b);
            b = next_b;
        }

        /* --- Phase 2 Free --- */
        // 
        free_book_index(g->bookindex);
        /* --- End Phase 2 Free --- */


        genre_t *next_g = g->next;
        free(g);
        g = next_g;
    }

    /* --- Phase 2 Free for Global Lists --- */
    // 
    member_activity_t *act = lib->activity;
    while (act != NULL) {
        member_activity_t *next_act = act->next;
        free(act);
        act = next_act;
    }

    // 
    free_recommendation_heap(lib->recommendations);
    /* --- End Phase 2 Free --- */


    lib->genres = NULL;
    lib->members = NULL;
    lib->activity = NULL; // 
    lib->recommendations = NULL; // 
}

// function to create an empty library
void init_library(library_t *lib) {
    lib->genres = NULL;
    lib->members = NULL;
    /* --- Phase 2 Initialization --- */
    lib->activity = NULL; //
    
    lib->recommendations = init_recommendation_heap(100); 
    if (lib->recommendations == NULL) {
        // 
        fprintf(stderr, "Failed to initialize recommendation heap\n");
        exit(1); 
    }

    lib->book_count = 0;
    lib->member_count = 0;
    lib->active_loans = 0;
    lib->total_score_sum = 0;
    lib->total_reviews_count = 0;
    /* --- End Phase 2 Init --- */


}


void init_book_data(book_t *book) {//used in insert_book function to initialize book fields
    book->sum_scores = 0;
    book->n_reviews = 0;
    book->avg = 0;
    book->lost_flag = 0;
    book->prev = NULL;
    book->next = NULL;
    book->heap_pos = -1; // shmainei oti den einai sto heap akoma
}



bool is_book_loaned_by_anyone(library_t *lib, int bid) { //  thn xrhsimopoiw gia na dw an to book me to bid einai daneiomeno 
    member_t *member = lib->members;
    while (member != NULL) {
        loan_t *loan = member->loans->next; // skip sentinel
        while (loan != NULL) {
            if (loan->bid == bid) {
                return true; // book is already loaned
            }
            loan = loan->next;
        }
        member = member->next;
    }
    return false; // book is not loaned by anyone
}




void insert_genre(library_t *lib, int gid, const char *name){

    genre_t *new_genre = (genre_t *)malloc(sizeof(genre_t));
    new_genre->gid = gid;
    strncpy(new_genre->name, name, NAME_MAX);
    new_genre->name[NAME_MAX-1] = '\0';
    new_genre->books = NULL;
    new_genre->lost_count = 0;
    new_genre->invalid_count = 0;
    new_genre->slots = 0;
    new_genre->display = NULL;
    new_genre->bookindex = NULL;
    new_genre->next = NULL;
    new_genre->rem = 0;

    // Insert in sorted order by gid
    if (lib->genres == NULL || lib->genres->gid > gid) {// an teleiwsei h lista h h thesh einai sthn arxh
        new_genre->next = lib->genres;
        lib->genres = new_genre;
    } else { // alliws briskoume th swsth thesh kai kanoume sorted insert by gid
        genre_t *current = lib->genres;
        while (current->next != NULL && current->next->gid < gid) {// h swsth thesh einai auth pou to epomeno genre exei megalutero gid apo to neo
            current = current->next;
        }
        new_genre->next = current->next;
        current->next = new_genre;
    }

}// event G - sorted insert by gid

void insert_book(library_t *lib, int bid, int gid, const char *title){
    genre_t *genre;
    if (!find_genre_gid(lib, gid, &genre)) {
       printf("IGNORED\n"); 
        return; // Genre not found
    }
    book_t *new_book = (book_t *)malloc(sizeof(book_t));
    new_book->bid = bid;
    new_book->gid = gid;
    strncpy(new_book->title, title, TITLE_MAX);
    new_book->title[TITLE_MAX-1] = '\0';
    init_book_data(new_book);
    genre->bookindex = avl_insert(genre->bookindex, new_book);
    lib->book_count++;
    

    // Insert book into genre's book list sorted by avg 
    if (genre->books == NULL) {
        genre->books = new_book;
    } else {
        book_t *current = genre->books;
        book_t *prev = NULL;
        while ( current != NULL && (current->avg > new_book->avg || (current->avg == new_book->avg && current->bid < new_book->bid))) { // taksinomisi fthinousa os pros avg kai an iso , mikrotero bid mpainei proto
            prev = current;
            current = current->next;
        }
        if (prev == NULL) {
            // Insert at the beginning
            new_book->next = genre->books;
            genre->books->prev = new_book;
            genre->books = new_book;
        } else {
            // Insert in the middle or end
            new_book->next = current;
            new_book->prev = prev;
            prev->next = new_book;
            if (current != NULL) { // an den einai to teleutaio
                current->prev = new_book; // update current prev pointer
            }
        }
    }

}//event BK

void insert_member(library_t *lib, int sid, const char *name){
    member_t *new_member = (member_t *)malloc(sizeof(member_t));
    new_member->sid = sid;
    strncpy(new_member->name, name, NAME_MAX);
    new_member->name[NAME_MAX-1] = '\0';
    
    // dimiourgia sentinel loan
    loan_t *sentinel = (loan_t *)malloc(sizeof(loan_t));
    sentinel->sid = sid;
    sentinel->bid = -1;     // shmainei oti den einai pragmatiko loan
    sentinel->next = NULL;
    new_member->loans = sentinel; // deixnei panta se sentinel

    new_member->next = NULL;
    lib->member_count++;

    // Sorted insert by sid
    if (lib->members == NULL || lib->members->sid > sid) { // an den yparxei member h to prwto exei megalutero sid tote einai sthn arxh
        new_member->next = lib->members;
        lib->members = new_member;
    } else { // alliws briskoume th swsth thesh kai kanoume sorted insert by sid
        member_t *current = lib->members;
        while (current->next != NULL && current->next->sid < sid) { // h swsth thesh einai auth pou to epomeno member exei megalutero sid apo to neo
            current = current->next;
        }
        new_member->next = current->next;
        current->next = new_member;
    }
}//event M - sorted insert by sid

void insert_loan(member_t *member, int sid, int bid){
    loan_t *new_loan = (loan_t *)malloc(sizeof(loan_t));
    new_loan->sid = sid;
    new_loan->bid = bid;

    // eisadogi meta sentinel
    new_loan->next = member->loans->next;
    member->loans->next = new_loan;
    lib.active_loans++;
    /* --- Phase 2 Member Activity Update (Loan) --- */
    member_activity_t *act_node = find_or_create_activity(&lib, sid, member->name);
    if (act_node != NULL) {
        act_node->loans_count++;  
    }
    /* --- End Phase 2 --- */
}//event L - unsorted insert sthn arxh ths listas loans

//delete function of loan
void event_R(member_t *member, int bid, int score, char *status){

    loan_t *prev_loan = member->loans;     // sentinel node
    loan_t *current_loan = member->loans->next;

    while (current_loan != NULL && current_loan->bid != bid) { // briskei to loan me to bid
        prev_loan = current_loan; // keep track to prohgoumeno loan
        current_loan = current_loan->next;//update current_loan
    }
    if (current_loan == NULL) {
        printf("IGNORED\n");
        return;
    }
    lib.active_loans--;

    if (strcmp(status, "lost") == 0) { // if status is lost update lost count lost flag and delete loan record
        // Update book statistics here 
        book_t *book;
        genre_t *genre;
        if (find_book_bid_all_genres(&lib, bid, &book) && find_genre_gid(&lib, book->gid, &genre)) {
            genre->lost_count += 1;
            book->lost_flag = 1;
        }
        /* --- Phase 2 Heap Remove (Lost) --- */
        if (book->heap_pos != -1) { // 
            heap_remove_by_pos(lib.recommendations, book->heap_pos);
        }
        /* --- End Phase 2 --- */

        // Remove loan from member's loan list
         
        prev_loan->next = current_loan->next;
        free(current_loan);
        printf("DONE\n");  
        return;


    } else if (strcmp(status, "ok") == 0 && score  == -1) { // score is NA delete loan reacord
        // Remove loan from member's loan list
        prev_loan->next = current_loan->next;
        free(current_loan);
        printf("DONE\n");  
        return;

    }else if (strcmp(status, "ok") == 0 && (score < 0 || score > 10 )) { // if score outside of range update invalid count and delete loan record
        book_t *book;
        genre_t *genre;

        

        if (find_book_bid_all_genres(&lib, bid, &book) && find_genre_gid(&lib, book->gid, &genre)) {
            genre->invalid_count += 1;// invalid count update
        }
        // Invalid score: ignore completely
        prev_loan->next = current_loan->next;
        free(current_loan);
        printf("IGNORED\n");
        return;

    }else if (strcmp(status, "ok") == 0 && score >= 0 && score <= 10) { // valid score update book statistics and delete loan record

        book_t *book;
        genre_t *genre;
        /* --- Phase 2 Member Activity Update (Review) --- */
        // 
        member_activity_t *act_node = find_or_create_activity(&lib, member->sid, member->name);
        if (act_node != NULL) {
            act_node->reviews_count++; 
            act_node->score_sum += score;   
        }
        /* --- End Phase 2 --- */
        if (find_book_bid_all_genres(&lib, bid, &book) && find_genre_gid(&lib, book->gid, &genre)) {
            book->sum_scores += score;
            book->n_reviews += 1;
            book->avg = (book->n_reviews > 0) ? (book->sum_scores / book->n_reviews) : 0;

            lib.total_score_sum += score;
            lib.total_reviews_count += 1;

            // Reposition book in genre's list based on new avg
            // prota, afairese to book apo thn lista
            if (book->prev != NULL) {// an den einai sthn arxh  
                book->prev->next = book->next;// update prev next pointer
            } else {
                genre->books = book->next; // book htan sthn arxikh thesh ara prepei na kanei update to head pointer
            }
            if (book->next != NULL) { // an den einai to teleutaio
                book->next->prev = book->prev;// update next prev pointer
            }



            // twra , update to book kai kane sorted insert pali sthn lista tou genre 
            book_t *current = genre->books;
            book_t *prev = NULL;
            while (current != NULL && (current->avg > book->avg ||(current->avg == book->avg && current->bid < book->bid))) { // taksinomisi fthinousa os pros avg kai an iso , to mikrotero bid mpainei proto
                prev = current;
                current = current->next;
            }
            if (prev == NULL) { // an einai na mpei sthn arxh tote prepei na kanei update kai to head pointer
                
                book->next = genre->books;
                book->prev = NULL;
                if (genre->books != NULL) {// an den einai to teleutaio tote prepei na kanei update tous pointers
                    genre->books->prev = book;// update prev pointer of old head
                }
                genre->books = book; // update head pointer
                
            } else { // alliws an einai mesa h sto telos prepei na kanei update tous pointers mono
                
                book->next = current;
                book->prev = prev;
                prev->next = book;
                if (current != NULL) {// an den einai to teleutaio
                    current->prev = book; // update current prev pointer 
                }
            }
            /* --- Phase 2 Heap Update (Valid Review) --- */
            if (book->heap_pos == -1) {
                
                heap_insert(lib.recommendations, book);
            } else {
                // 
                heap_update_key(lib.recommendations, book->heap_pos);
            }
            /* --- End Phase 2 --- */


        }
        prev_loan->next = current_loan->next;
        free(current_loan);
        printf("DONE\n");  
        return;
    }

    printf("IGNORED\n");
    
}




//Display function(s) kai alles synarthseis gia kathe event 
void event_D(library_t *lib) {
    if (SLOTS <= 0) {// special case otan SLOTS <= 0 tote ola ta genres exoun 0 slots kai display = NULL
        genre_t *g = lib->genres;
        while (g) {
            g->slots = 0;
            g->display = NULL;
            g = g->next;
        }
        return;
    }

    // ypologismos points ana genre
    genre_t *g = lib->genres;
    int total_points = 0; // synolika points gia ola ta genres 
    
    while (g) { // gia kathe genre ypologizw ta points kai arxikopoihsw slots = 0
        int points = 0;
        book_t *b = g->books;
        while (b) {
            if (b->n_reviews > 0 && b->lost_flag == 0) { // mono an exei reviews kai den einai lost
                points += b->sum_scores; // prosthetw ta scores
            }
            b = b->next;
        }
        g->rem = points; // prosorini apothikeysi twn points wste na mporw na ta xrhsimopoihsw gia to quota
        g->slots = 0;
        total_points += points;
        g = g->next;
    }

    if (total_points == 0) { // an den yparxoun points se kanena genre tote ola exoun 0 slots kai display = NULL
        g = lib->genres;
        while (g) {
            g->slots = 0;
            g->display = NULL;
            g = g->next;
        }
        return;
    }

    // ypologismos quota
    int quota = total_points / SLOTS; 
    
    
    // elegxos gia quota = 0 prin apo to loop (den anaferetai sto ppt alla ekana dikia mou ylopoihsh)
    if (quota == 0) {  //an quota = 0 shmainei oti total_points < SLOTS opote ta synolika slots einai megalutera apo ta points olwn twn genres ara prepei na dwsw se kathe genre me points > 0 to megisto plithos slots pou mporei na parei (1)
        // Special case otan total_points < SLOTS
        // dose 1 slot se kathe genre me points > 0 mexri na teleiwsoun ta slots
        
        int slots_given = 0;
        g = lib->genres;
        while (g && slots_given < SLOTS) { // an yparxoun slots na dosei
            if (g->rem > 0) { // an exei rem to genre tote dose tou 1 slot
                g->slots = 1;
                g->rem = 0;
                slots_given++;
            }
            g = g->next;
        }
        // ta ypoloipa genres pairnoun 0 slots
        while (g) { // gia ta ypoloipa genres
            g->slots = 0; // arxikopoihsh slots = 0
            g->rem = 0;
            g = g->next;
        }
    } else {
        // kanomiki quota > 0
        int assigned = 0;
        g = lib->genres;
        while (g) {// protos gyros diahrisis me to quota
            int points = g->rem; // pairnw ta points apo to rem
            g->slots = points / quota; // arithmos slots me vash to quota
            g->rem = points - (g->slots * quota); // ypoloipo
 
            assigned += g->slots; // synolika assigned slots gia ton epomeno gyro
            g = g->next;
        }
       
        // Largest remainder method se isobathmia to genre me mikrotero gid pairnei to slot 
        int remaining = SLOTS - assigned;
        while (remaining > 0) { // oso yparxoun ypoloipa slots na dosei
            genre_t *best = NULL; // sto best apothikeyw to genre me to megalytero rem 
            int best_rem = -1; // arxikopoihsh me -1 gia na mporoume na kanoume swsto elegxo ekei tha yparxei to megalytero rem
            
            g = lib->genres;
            while (g) { // vriskw to genre me to megalytero rem
                if (g->rem > best_rem || (g->rem == best_rem && (best == NULL || g->gid < best->gid))) { // an rem einai megalutero h iso kai me mikrotero gid apo to best
                    best_rem = g->rem; // to best rem einai to rem tou genre kai to best einai to genre
                    best = g;
                }
                g = g->next;
            }
            
            if (best == NULL || best_rem < 0){break;}  // den yparxei allo genre gia na dosei slot
            
            best->slots += 1; // dose to slot sto genre me to megalytero rem
            best->rem = -1; // orizw to rem se -1 oste na min ksanadwsei exei parei hdh slot
            remaining--;
        }
    }

    // Display pointers , edw orizw to display pointer gia kathe genre
    g = lib->genres;
    while (g) { // gia kathe genre
        if (g->slots <= 0) { // an exei 0 slots tote display = NULL
            g->display = NULL;
        } else { // alliws briskw to prwto book pou na mporouse na emfanistei sto display
            book_t *b = g->books;
            while (b && (b->n_reviews == 0 || b->lost_flag == 1)) { // pernaei books mexri na vrei ena pou exei reviews kai den einai lost
                b = b->next;
            }
            g->display = b;// orizw to display pointer ws to prwto book pou mporouse na emfanistei 
        }
        g = g->next;
    }
}



void event_PD(library_t *lib){

    genre_t *current_genre = lib->genres;
    printf("Display:\n");
    while (current_genre != NULL) { // gia kathe genre
        printf("%d:\n", current_genre->gid);

        book_t *current_book = current_genre->books;
        int displayed = 0;
        while (current_book != NULL && displayed < current_genre->slots) { // emfanise mexri to plithos twn slots
            if (current_book->n_reviews > 0 && current_book->lost_flag == 0) {
                printf(" %d,%d \n", current_book->bid, current_book->avg);
                displayed++;
            }
            current_book = current_book->next;
        }
        current_genre = current_genre->next;
    }
    
    
    //print display
    
}




void event_PG(library_t *lib, int gid){//print genres, prints all the books of a genre by avg desc <bid> , <avg>
    genre_t *genre;
    if (!find_genre_gid(lib, gid, &genre)) { // an den vrethei to genre me to gid tote print IGNORED
        printf("IGNORED\n");
        return;
    }
    printf("DONE\n");
    book_t *current_book = genre->books;
    while (current_book != NULL) { // gia kathe book tou genre
        printf(" %d,%d\n",current_book->bid,current_book->avg); // print bid , avg
        current_book = current_book->next;
    }
}



void event_PM(library_t *lib,int sid ){//print members, prints all loans of a member <bid1> <bid2> ...<bidN>
    member_t *member;
    if (!find_member_sid(lib, sid, &member)) {
        printf("IGNORED\n");
        return;
    }
    printf("Loans:\n");
    loan_t *current_loan = member->loans->next;
    while (current_loan != NULL) {
        printf("%d\n", current_loan->bid);
        current_loan = current_loan->next;
    }
    

}




void event_S(int slots){ SLOTS = slots;}//orizei to synoliko plhthos SLOTS

/* Print simple stats: per-genre lost_count/invalid_count and slots (for PS command) */
void event_PS(library_t *lib){
    printf("Stats:\n");
    genre_t *g = lib->genres;
    while (g != NULL) {
        printf("Genre %d \"%s\": lost=%d invalid=%d slots=%d\n",g->gid, g->name, g->lost_count, g->invalid_count, g->slots);
        g = g->next;
    }
}


bool find_book_bid_all_genres(library_t *lib, int bid, book_t **out_book) { // briskei to book me to bid se ola ta genres 
    genre_t *genre = lib->genres;
    while (genre != NULL) {
        book_t *book = genre->books;
        while (book != NULL) {
            if (book->bid == bid) {
                if (out_book != NULL) {
                    *out_book = book;
                }
                return true;
            }
            book = book->next;
        }
        genre = genre->next;
    }
    return false;
}

bool find_genre_gid(library_t *lib, int gid, genre_t **out_genre){ // briskei to genre me to gid

    genre_t *current = lib->genres;
    while (current != NULL) {
        if (current->gid == gid) {
            if (out_genre != NULL) {
                *out_genre = current;
            }
            return true;
        }
        current = current->next;
    }
    return false;



}//synarthsh gia na vrw to genre me to gid




bool find_member_sid(library_t *lib, int sid, member_t **out_member){ // briskei to member me to sid anamesa se ola ta members

    member_t *current = lib->members;
    while (current != NULL) {
        if (current->sid == sid) {
            if (out_member != NULL) {
                *out_member = current;
            }
            return true;
        }
        current = current->next;
    }
    return false;




}//synarthsh gia na vrw to member me to sid




bool find_book_bid_genre(genre_t *genre, int bid, book_t **out_book){ // briskei to book me to bid se ena genre pou dinei ws orisma

    
    book_t *current_book = genre->books;
    while (current_book != NULL) {
        if (current_book->bid == bid) {
            if (out_book != NULL) {
                    *out_book = current_book;
            }
            return true;
        }
        current_book = current_book->next;
    }
    return false;




}//synarthsh gia na vrw to book me to bid se ena genre




bool find_loan_bid_member(member_t *member, int bid, loan_t **out_loan){ // briskei to loan me to bid se ena member

    loan_t *current = member->loans->next; // skip sentinel
    while (current != NULL) {
        if (current->bid == bid) {
            if (out_loan != NULL) {
                *out_loan = current;
            }
            return true;
        }
        current = current->next;
    }
    return false;


}//synarthsh gia na vrw to loan me to bid se ena member




//phase 2 helpers 
//(1) oti xreiazetai gia to booKindex kai to avl tree tou

int height(book_index_t *node) {
    if (node == NULL)
        return 0;
    return node->height;
}

// 
int maxf(int a, int b) {
    return (a > b) ? a : b;
}


book_index_t* new_avl_node(book_t *book) {
    book_index_t *node = (book_index_t*)malloc(sizeof(book_index_t));
    strncpy(node->title, book->title, TITLE_MAX);
    node->title[TITLE_MAX-1] = '\0';
    node->book = book; 
    node->lc = NULL;
    node->rc = NULL;
    node->height = 1; 
    return node;
}


book_index_t* right_rotate(book_index_t *y) {
    book_index_t *x = y->lc;
    book_index_t *T2 = x->rc;

    x->rc = y;
    y->lc = T2;
    
    y->height = maxf(height(y->lc), height(y->rc)) + 1;
    x->height = maxf(height(x->lc), height(x->rc)) + 1;

    return x;
}


book_index_t* left_rotate(book_index_t *x) {
    book_index_t *y = x->rc;
    book_index_t *T2 = y->lc;

    y->lc = x;
    x->rc = T2;

    x->height = maxf(height(x->lc), height(x->rc)) + 1;
    y->height = maxf(height(y->lc), height(y->rc)) + 1;

    return y;
}


int get_balance(book_index_t *node) {
    if (node == NULL)
        return 0;
    return height(node->lc) - height(node->rc);
}


book_index_t* avl_insert(book_index_t *node, book_t *book) {

    if (node == NULL)
        return(new_avl_node(book));

    int cmp = strcmp(book->title, node->title);

    if (cmp < 0){
        node->lc = avl_insert(node->lc, book);
    } else if (cmp > 0){
        node->rc = avl_insert(node->rc, book);
    } else {
        return node;
    }
    node->height = 1 + maxf(height(node->lc), height(node->rc));
    
    int balance = get_balance(node);

    // aristeri aristeri
    if (balance > 1 && strcmp(book->title, node->lc->title) < 0){ 
        return right_rotate(node);
    }

    // deksia deksia
    if (balance < -1 && strcmp(book->title, node->rc->title) > 0){
        return left_rotate(node);
    }

    // aristeri deksia
    if (balance > 1 && strcmp(book->title, node->lc->title) > 0) {
        node->lc = left_rotate(node->lc);
        return right_rotate(node);
    }

    // deksia aristeri
    if (balance < -1 && strcmp(book->title, node->rc->title) < 0) {
        node->rc = right_rotate(node->rc);
        return left_rotate(node);
    }

    
    return node;
}


book_index_t* avl_search(book_index_t *root, const char *title) {
    if (root == NULL) {
        return NULL; 
    }

    int cmp = strcmp(title, root->title);

    if (cmp == 0) {
        return root; 
    } else if (cmp < 0) {
        return avl_search(root->lc, title); 
    } else {
        return avl_search(root->rc, title); 
    }
}


book_index_t* avl_min_node(book_index_t *node) {
    book_index_t *current = node;
    while (current->lc != NULL)
        current = current->lc;
    return current;
}

book_index_t* avl_delete(book_index_t *root, const char *title) {
    if (root == NULL){
        return root;
    }
    int cmp = strcmp(title, root->title);
    if (cmp < 0)
        root->lc = avl_delete(root->lc, title);
    else if (cmp > 0)
        root->rc = avl_delete(root->rc, title);
    else {
    
        if (root->lc == NULL || root->rc == NULL) {
            book_index_t *temp = root->lc ? root->lc : root->rc;
            if (temp == NULL) { 
                temp = root;
                root = NULL;
            } else { 
                *root = *temp;  
            }
            free(temp);
        } else {
            
            book_index_t *temp = avl_min_node(root->rc);
            
            strncpy(root->title, temp->title, TITLE_MAX);
            root->book = temp->book;
            
            root->rc = avl_delete(root->rc, temp->title);
        }
    }


    if (root == NULL){
        return root;
    }
    root->height = 1 + maxf(height(root->lc), height(root->rc));
    int balance = get_balance(root);

    // aristeri aristeri
    if (balance > 1 && get_balance(root->lc) >= 0)
        return right_rotate(root);
    // aristeri deksia
    if (balance > 1 && get_balance(root->lc) < 0) {
        root->lc = left_rotate(root->lc);
        return right_rotate(root);
    }
    // deksia deksia
    if (balance < -1 && get_balance(root->rc) <= 0)
        return left_rotate(root);
    // deksia aristeri
    if (balance < -1 && get_balance(root->rc) > 0) {
        root->rc = right_rotate(root->rc);
        return left_rotate(root);
    }

    return root;
}

//(2) pame sthn ylopoihsh tou recommendation heap kai olwn twn helpers

recommendation_heap_t* init_recommendation_heap(int capacity) {
    recommendation_heap_t *h = (recommendation_heap_t*)malloc(sizeof(recommendation_heap_t));
    if (h == NULL) return NULL;
    
    h->heap = (book_t**)malloc(capacity * sizeof(book_t*));
    if (h->heap == NULL) {
        free(h);
        return NULL;
    }
    
    h->size = 0;
    h->capacity = capacity;
    return h;
}


void heap_swap(recommendation_heap_t *h, int i, int j) {
    book_t *temp = h->heap[i];
    
    h->heap[i] = h->heap[j];
    h->heap[j] = temp;
    
    h->heap[i]->heap_pos = i;
    h->heap[j]->heap_pos = j;
}


void heapify_up(recommendation_heap_t *h, int i) {
    if (i == 0) return; 
    
    int parent = (i - 1) / 2;

    if (h->heap[i]->avg > h->heap[parent]->avg) {
        heap_swap(h, i, parent);
        heapify_up(h, parent);  
    }
}

 
void heapify_down(recommendation_heap_t *h, int i) {
    int left = 2 * i + 1;
    int right = 2 * i + 2;
    int largest = i;

    if (left < h->size && h->heap[left]->avg > h->heap[largest]->avg) {
        largest = left;
    }

    if (right < h->size && h->heap[right]->avg > h->heap[largest]->avg) {
        largest = right;
    }

    if (largest != i) {
        heap_swap(h, i, largest);
        heapify_down(h, largest); 
    }
}


void heap_grow(recommendation_heap_t *h) {
    if (h->size >= h->capacity) {
        h->capacity = (h->capacity == 0) ? 16 : h->capacity * 2; 
        h->heap = (book_t**)realloc(h->heap, h->capacity * sizeof(book_t*));
        
    }
}


void heap_insert(recommendation_heap_t *h, book_t *book) {
    if (book->lost_flag == 1) return; 
    
    heap_grow(h); 
    
    int i = h->size;
    h->heap[i] = book;
    book->heap_pos = i;
    h->size++;
    
    heapify_up(h, i); 
}


void heap_update_key(recommendation_heap_t *h, int i) {
    if (i < 0 || i >= h->size) return; 
    
   
    int parent = (i - 1) / 2;
    if (i > 0 && h->heap[i]->avg > h->heap[parent]->avg) {
        heapify_up(h, i);
    } else {
        heapify_down(h, i);
    }
}


book_t* heap_remove_by_pos(recommendation_heap_t *h, int i) {
    if (i < 0 || i >= h->size) return NULL;
    
    book_t *book_to_remove = h->heap[i];
    
  
    heap_swap(h, i, h->size - 1);
    h->size--;
    
    book_to_remove->heap_pos = -1;
    
    heap_update_key(h, i);
    
    return book_to_remove;
}


//(3) KAI TELEUTAIA , OTI XREIAZETAI GIA TO MEMBER ACTIVITY KAI TA EVENTS KAI HELPERS 

member_activity_t* find_or_create_activity(library_t *lib, int sid, const char *name) {
    
    member_activity_t *current = lib->activity;
    while (current != NULL) {
        if (current->sid == sid) {
            return current;
        }
        current = current->next;
    }

    member_activity_t *new_node = (member_activity_t*)malloc(sizeof(member_activity_t));
    if (new_node == NULL) {
        fprintf(stderr, "Failed to allocate memory for MemberActivity node\n");
        return NULL;  
    }
    
    new_node->sid = sid;

    strncpy(new_node->name, name, NAME_MAX);
    new_node->name[NAME_MAX-1] = '\0';

    new_node->loans_count = 0;
    new_node->reviews_count = 0;
    new_node->score_sum = 0;
    
    new_node->next = lib->activity;
    lib->activity = new_node;
    
    return new_node;
}





int main(int argc, char *argv[]){
    if (argc < 2) {//checkarei an exei dwthei arxeio eisodou kai an oxi emfanizei minima
        fprintf(stderr, "Usage: %s <inputfile>\n", argv[0]);
        return 1;
    }
    char *inputfilename = argv[1];//to onoma tou arxeio einai to prwto orisma
    FILE *inputfile = fopen(inputfilename, "r");// anoigma arxeiou gia readi
    if (inputfile == NULL) {
        fprintf(stderr, "Error opening file: %s\n", inputfilename);// an den mporei na anoiksei to arxeio emfanizei minima sfalmatos
        return 1;
    }
    init_library(&lib);//arxikopoihsh tou library
    char line[TITLE_MAX];// pinaka gia na diavasei grammes apo to arxeio 
    while (fgets(line, sizeof(line), inputfile)) {
        // skip empty or comment lines
        char *p = line;// diavasei mia grammi apo to arxeio kai thn apothikeuei ston pinaka line
        while (*p == ' ' || *p == '\t') p++;// an h grammi arxizei me kena h tab paei sto epomeno xarakthra
        if (*p == '\n' || *p == '\0' || *p == '#') continue;//an h grammi einai adeia h einai sxolio paei stin epomenh grammi

        char cmd[16];// pinaka gia na apothikeuei to command dld to prwto string ths grammhs
        if (sscanf(line, " %15s", cmd) != 1) continue;// diavasei to prwto string ths grammhs kai to apothikeuei sto cmd an den yparxei paei stin epomenh grammi

        if (strcmp(cmd, "G") == 0) {//an to command einai G tote diavasei to gid kai to onoma kai kalei thn insert_genre
            int gid;
            char name[NAME_MAX];
            if (sscanf(line, "G %d \"%[^\"]\"", &gid, name) == 2) {//diavasei to gid kai to onoma apo thn grammh//ta symbola %[^\"] ta agnoei mexri na synanththei to " kai apothikeuei to onoma(mr bohthhse to copilot edw ligo )
                bool found = find_genre_gid(&lib, gid, NULL);
                if (found){ printf("IGNORED\n");continue;} // genre already exists, skip insertion
                insert_genre(&lib, gid, name);
                printf("DONE\n");

            }
        } else if (strcmp(cmd, "BK") == 0) {
            int bid, gid;
            char title[TITLE_MAX];
            if (sscanf(line, "BK %d %d \"%[^\"]\"", &bid, &gid, title) == 3) {
                
                genre_t *genre; // 
                if (!find_genre_gid(&lib, gid, &genre)) { 
                    printf("IGNORED\n"); 
                    continue; 
                }

                bool book_found = find_book_bid_all_genres(&lib, bid, NULL);
                if (book_found){printf("IGNORED\n"); continue; }// 

                /* --- Phase 2 Title Check --- */
                // 
                if (avl_search(genre->bookindex, title) != NULL) {
                    printf("IGNORED\n"); // 
                    continue;
                }
                /* --- End Phase 2 --- */
                insert_book(&lib, bid, gid, title);
                printf("DONE\n");

            }
        } else if (strcmp(cmd, "M") == 0) {
            int sid;
            char name[NAME_MAX];
            if (sscanf(line, "M %d \"%[^\"]\"", &sid, name) == 2) {
                bool found = find_member_sid(&lib, sid, NULL);
                if (found){printf("IGNORED\n"); continue;} // member already exists, skip insertion
                insert_member(&lib, sid, name);
                printf("DONE\n");

            }
        } else if (strcmp(cmd, "L") == 0) {
            int sid, bid;
            if (sscanf(line, "L %d %d", &sid, &bid) == 2) {
                member_t *member;
                if (find_member_sid(&lib, sid, &member) && find_book_bid_all_genres(&lib, bid, NULL) && !is_book_loaned_by_anyone(&lib, bid)) { // ama yparxei to member kai to book kai den yparxei hdh o daneiismos apo allo atomo kane insert
                    insert_loan(member, sid, bid);
                    printf("DONE\n");
                } else {
                    printf("IGNORED\n");
                    continue; //alliws skiparei to insert
                }
            }
        } else if (strcmp(cmd, "R") == 0) {
            int sid, bid;
            char score_tok[16];
            char status[32];
            // read score as token (numeric or "NA") / me ligh bohtheia apo copilot 
            if (sscanf(line, "R %d %d %15s %31s", &sid, &bid, score_tok, status) >= 3) {//to score einai eite int eite "NA"
                status[strcspn(status, "\r\n")] = '\0'; // remove newline apo to status
                int score = -1; // -1 shmainei "NA"
                if (strcmp(score_tok, "NA") != 0) { // if score is not "NA", convert to integer
                    char *end;
                    long v = strtol(score_tok, &end, 10);// converts string to long
                    if (*end != '\0') {printf("IGNORED\n");continue;} /* invalid score token, skip line */
                    score = (int)v;// safe to cast to int afou elegxthke parapanw
                }
                member_t *member;
                if (find_member_sid(&lib, sid, &member) && find_book_bid_all_genres(&lib, bid, NULL) && find_loan_bid_member(member, bid, NULL)) {
                    event_R(member, bid, score, status);

                }else {
                    printf("IGNORED\n");
                    continue; //alliws skiparei to return
            }
        }
        } else if (strcmp(cmd, "D") == 0) {
            event_D(&lib);
            printf("DONE\n");

        } else if (strcmp(cmd, "PD") == 0) {
            event_PD(&lib);
            printf("DONE\n");

        } else if (strcmp(cmd, "PG") == 0) {
            int gid;
            if (sscanf(line, "PG %d", &gid) == 1) {
                event_PG(&lib, gid);


            }
        } else if (strcmp(cmd, "PM") == 0) {
            int sid;
            if (sscanf(line, "PM %d", &sid) == 1) {
                event_PM(&lib, sid);
                printf("DONE\n");

            }
        } else if (strcmp(cmd, "S") == 0) {
            int slots;
            if (sscanf(line, "S %d", &slots) == 1) {
                event_S(slots);
                printf("DONE\n");

            }
        } else if (strcmp(cmd, "PS") == 0) {
            event_PS(&lib);
            printf("DONE\n");
            /* === PHASE 2 EVENTS === */
        } else if (strcmp(cmd, "F") == 0) {
            // F <title>
            char title[TITLE_MAX];
            if (sscanf(line, "F \"%[^\"]\"", title) == 1) {
                book_index_t *found_node = NULL;
                genre_t *g = lib.genres;
                
                while (g != NULL) {
                    found_node = avl_search(g->bookindex, title);
                    if (found_node != NULL) {
                        break; 
                    }
                    g = g->next;
                }

                if (found_node != NULL) {
                    
                    book_t *b = found_node->book;
                    printf("DONE\n");
                    printf(" bid=%d, gid=%d, avg=%d, lost=%d, \"%s\"\n",
                           b->bid, b->gid, b->avg, b->lost_flag, b->title);
                } else {
                    printf("NOT FOUND\n"); 
                }

            } else {
                printf("IGNORED\n");
            }

        } else if (strcmp(cmd, "TOP") == 0) {
            // TOP <k>
            int k;
            if (sscanf(line, "TOP %d", &k) == 1) {
                printf("DONE\n");
                
                if (k <= 0) {
                    continue; 
                }

                recommendation_heap_t *h = lib.recommendations;

                int count = 0;
                book_t **temp_storage;
                
                int n_to_pop = (k > h->size) ? h->size : k;
                
                temp_storage = (book_t**)malloc(n_to_pop * sizeof(book_t*));
                if (temp_storage == NULL) {
                    printf("IGNORED\n"); 
                    continue;
                }
                // 1
                for (count = 0; count < n_to_pop; count++) {
                    book_t *top_book = h->heap[0]; 
                    
                    printf(" %d,\"%s\",%d\n", top_book->bid, top_book->title, top_book->avg);
                    
                    temp_storage[count] = heap_remove_by_pos(h, 0); 
                }
                // 2
                for (int i = 0; i < count; i++) {
                    heap_insert(h, temp_storage[i]);
                }
                
                free(temp_storage);

            } else {
                printf("IGNORED\n");
            }

        } else if (strcmp(cmd, "AM") == 0) {

            // AM
            printf("DONE\n");
            member_activity_t *current = lib.activity;
            
            if (current == NULL) {
                printf("NO ACTIVE MEMBERS\n"); 
            } else {
                printf("MemberActivity:\n");
                while (current != NULL) {
                    
                    int avg = 0;
                    if (current->reviews_count > 0) {
                        avg = current->score_sum / current->reviews_count;
                    }
                    printf(" sid=%d, name=%s, loans=%d, reviews=%d, avg=%d\n", current->sid, current->name, current->loans_count, current->reviews_count, avg);
                    current = current->next;
                }
            }

        } else if (strcmp(cmd, "U") == 0) {
            // U <bid> "<new_title>"
            int bid;
            char new_title[TITLE_MAX];
            if (sscanf(line, "U %d \"%[^\"]\"", &bid, new_title) == 2) {
                book_t *book = NULL;
                genre_t *genre = NULL;

              
                if (!find_book_bid_all_genres(&lib, bid, &book)) {
                    printf("IGNORED\n");
                    continue;
                }
            
                if (!find_genre_gid(&lib, book->gid, &genre)) {
                    printf("IGNORED\n");  
                    continue;
                }

                if (avl_search(genre->bookindex, new_title) != NULL) {
                    printf("IGNORED\n");  
                    continue;
                }

                genre->bookindex = avl_delete(genre->bookindex, book->title);

                strncpy(book->title, new_title, TITLE_MAX);
                book->title[TITLE_MAX-1] = '\0';
                genre->bookindex = avl_insert(genre->bookindex, book);
                
                printf("DONE\n");

            } else {
                printf("IGNORED\n");
            }

        } else if (strcmp(cmd, "X") == 0) {
            // X
            printf("DONE\n");
            printf("System Stats:\n");
            
            int all_avg = 0;
            if (lib.total_reviews_count > 0) {
                all_avg = lib.total_score_sum / lib.total_reviews_count;
            }
            
            printf(" books=%d\n", lib.book_count);         
            printf(" members=%d\n", lib.member_count);     
            printf(" loans=%d\n", lib.active_loans);       
            printf(" all_avg_score=%d\n", all_avg);       

        } else if (strcmp(cmd, "BF") == 0) {
            // BF
            free_all(&lib);
            // to exw kanei meta ta list me th synarthsh free_all opote tha ginei outws h allws
            printf("DONE\n");
            break; 

        } else {
            fprintf(stderr, "Unknown command: %s\n", cmd);
        }
    }
    fclose(inputfile);
    //free_all(&lib);
    return 0;


}

