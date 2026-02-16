/* =========================
   Κοινά & βοηθητικά
   ========================= */
#ifndef LIBRARY_H
#define LIBRARY_H

#include <stddef.h>

#define TITLE_MAX 128
#define NAME_MAX  64

/*  Παγκόσμια θέση προβολών (ορίζεται από την εντολή S <slots>)
    Φροντίστε να το κάνετε define στο αντίστοιχο .c αρχείο, ως
    int SLOTS;
*/
extern int SLOTS;
/* -----------------------------------------
   BOOK INDEX (Phase 2): AVL Tree Node
   - Ταξινομημένο κατά title (lexicographically) 
   - Κάθε κόμβος δείχνει στο αντίστοιχο book_t 
   ----------------------------------------- */
typedef struct book_index {
   char title[TITLE_MAX];          // 
   struct book *book;              // Δείκτης στο κανονικό struct book 
   int height;                     // Ύψος AVL 
   struct book_index *lc;          // Αριστερό παιδί 
   struct book_index *rc;          // Δεξί παιδί 
} book_index_t;

/* -----------------------------------------
   MEMBER ACTIVITY (Phase 2): Linked List Node
   - Μονοσύνδετη, μη ταξινομημένη λίστα 
   - Παρακολουθεί δάνεια και κριτικές ανά μέλος 
   ----------------------------------------- */
typedef struct member_activity {
   int sid;  
   char name[NAME_MAX];                      
   int loans_count;                
   int reviews_count;              
   int score_sum;                  
   struct member_activity *next;   
} member_activity_t;

/* -----------------------------------------
   RECOMMENDATION HEAP (Phase 2): Max Heap
   - Max Heap ταξινομημένος κατά book->avg 
   - Αποθηκεύει δείκτες σε book_t structs
   ----------------------------------------- */
typedef struct recommendation_heap {
    struct book **heap;             // Πίνακας από δείκτες σε book_t 
    int size;                       // Τρέχον μέγεθος heap 
    int capacity;                   // Μέγιστη χωρητικότητα 
} recommendation_heap_t;


/* -----------------------------------------
   LOAN: ενεργός δανεισμός (unsorted, O(1) insert/remove)
   Λίστα ανά Member με χρήση sentinel node.
   ----------------------------------------- */
typedef struct loan {
    int sid;            /* member id (ιδιοκτήτης της λίστας) */
    int bid;            /* book id που έχει δανειστεί */
    struct loan *next;  /* επόμενος δανεισμός του μέλους */
} loan_t;

/* -----------------------------------------
   BOOK: βιβλίο
   - Ανήκει σε ακριβώς ένα Genre (gid)
   - Συμμετέχει στη διπλά συνδεδεμένη λίστα του Genre,
     ταξινομημένη φθίνοντα κατά avg.
   ----------------------------------------- */
typedef struct book {
   int  bid;                         /* book id (μοναδικό) */
   int  gid;                         /* genre id (ιδιοκτησία λίστας) */
   char title[TITLE_MAX];

   /* Στατιστικά δημοτικότητας */
   int sum_scores;                   /* άθροισμα έγκυρων βαθμολογιών */
   int n_reviews;                    /* πλήθος έγκυρων βαθμολογιών */
   int avg;                          /* cache: floor(sum_scores / n_reviews); 0 αν n_reviews=0 */
   int lost_flag;                    /* 1 αν δηλωμένο lost, αλλιώς 0 */

   /* Διπλά συνδεδεμένη λίστα του genre, ταξινομημένη κατά avg (desc). */
   struct book *prev;
   struct book *next;
   /* ==  FOR PHASE 2 == */
   int heap_pos; /* Η θέση του βιβλίου στον Recommendation Heap  */

   /* Προαιρετικό: συνδέσεις σε global ευρετήρια αν κρατήσετε (όχι απαραίτητο) */
   // struct book *next_global;         /* π.χ. unsorted λίστα όλων των βιβλίων */
} book_t;

/* -----------------------------------------
   MEMBER: μέλος βιβλιοθήκης
   - Κρατά unsorted λίστα ενεργών δανεισμών (loan_t) με χρήση sentinel node
   ----------------------------------------- */
typedef struct member {
   int  sid;                         /* member id (μοναδικό) */
   char name[NAME_MAX];

   /* Λίστα ενεργών δανεισμών:
    Uns. singly-linked με sentinel node:
    - Εισαγωγή: O(1) push-front
    - Διαγραφή γνωστού bid: O(1) αν κρατάτε prev pointer στη σάρωση */
   loan_t* loans;

   /* Μονοσυνδεδεμένη λίστα όλων των μελών ταξινομημένη κατά sid */
   struct member *next;
} member_t;

/* -----------------------------------------
   GENRE: κατηγορία βιβλίων
   - Κρατά ΔΙΠΛΑ συνδεδεμένη λίστα ΒΙΒΛΙΩΝ ταξινομημένη κατά avg (desc)
   - Κρατά και το αποτέλεσμα της τελευταίας D (display) για εκτύπωση PD
   ----------------------------------------- */
typedef struct genre {
   int  gid;                         /* genre id (μοναδικό) */
   char name[NAME_MAX];

   /* διπλά συνδεδεμένη λίστα βιβλίων ταξινομημένη κατά avg φθίνουσα. */
   book_t* books;
   int lost_count;
   int invalid_count;

   /* Αποτέλεσμα τελευταίας κατανομής D: επιλεγμένα βιβλία για προβολή.
      Αποθηκεύουμε απλώς pointers στα book_t (δεν αντιγράφουμε δεδομένα). */
   int slots;            /* πόσα επιλέχθηκαν για προβολή σε αυτό το genre */
   book_t *display;     //allaksa se sketo *display gia na deixnei se book_t*

   /* Μονοσυνδεδεμένη λίστα όλων των genres ταξινομημένη κατά gid (για εύκολη σάρωση). */
   struct genre *next;
   int rem;    /* προσωρινή αποθήκευση υπολοίπου (points - seats*quota) για D */
   book_index_t *bookindex;  // Δείκτης στη ρίζα του AVL tree           
} genre_t;

/* -----------------------------------------
   LIBRARY: κεντρικός "ρίζας"
   - Κρατά λίστα Genres (sorted by gid)
   - Κρατά λίστα Members (sorted by sid)
   ----------------------------------------- */
typedef struct library {
    genre_t  *genres;     /* κεφαλή λίστας genres (sorted by gid) */
    member_t *members;    /* διπλά συνδεδεμένη λίστα μελών (sorted by sid) */
    //book_t   *books;      /* unsorted λίστα όλων των books (ευκολία αναζήτησης) — προαιρετικό */
    
    /* == NEW FIELDS FOR PHASE 2 == */
    member_activity_t *activity;        // Κεφαλή της λίστας activity 
    recommendation_heap_t *recommendations; // Δείκτης στο struct του heap

    /* == NEW FIELDS FOR SYSTEM STATS (Phase 2) == */
    int book_count;     // 
    int member_count;   // 
    int active_loans;   // 

    /* Για τον υπολογισμό του all_score_avg χρειαζόμαστε το σύνολο και το πλήθος */
    int total_score_sum;  // 
    int total_reviews_count; //
} library_t;




#endif
