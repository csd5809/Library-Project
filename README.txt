Project: Systhma Diaxeirisis Vivliothikis 
Onoma : Xristoforakis Xristoforos

=============================================================================
1. METAFRASI & EKTELESH (COMPILATION & EXECUTION)
=============================================================================

Gia na kanete compile to programma me ola ta warnings energopoihmena (proteinomeno) 
kai me plirofories aposphalmatwsis (debug info), xrisimopoihste thn parakatw 
entoli sto terminal:

    gcc -o main main.c -Wall -Wextra -g

    kanei compile kanonika kai me sketo : gcc -o main main.c

Gia na treksete to programma me ena arxeio eisodou (p.x. test3.txt):

    ./main test3.txt

ta arxeia 'Library.h' kai 'main.c' prepei na briskontai ston idio fakelo.

=============================================================================
2.  OVERVIEW
=============================================================================

Sth Fasi 2, to Systhma Diaxeirisis Vivliothikis epektathike gia na ypostirizei 
proxorimenes dynatotites anazitisis, systhma proteinomenwn vivliwn (recommendations) 
kai katagrafi drastiriotitas melwn. Auto epiteuxthike me thn ensomatwsi triwn 
newn domwn dedomenwn dipla stis yparxouses listes ths Fasis 1.

=============================================================================
3. YLOPOIHMENES DOMES DEDOMENWN 
=============================================================================

A. BookIndex (AVL Tree)
-----------------------
- Perigrafi: Ena AVL Dendro ylopoiimeno gia *kathe* Genre (Kathgoria).
- Skopos: Epitreptei grigori anazitisi O(log n) twn vivliwn me vasi ton titlo 
  (leksikografika).
- Ylopoiisi: 
  - Kathe komvos (book_index_t) periexei deikti pros to pragmatiko book_t.
  - Ylopoiithikan oi klasikes peristrofes AVL (Right, Left, Right-Left, Left-Right) 
    gia na diatireitai h isorropia tou dendrou.
  - Xrisimopoieitai sta events: 'F' (Find) kai 'U' (Update title).

B. Recommendation Heap (Max Heap)
---------------------------------
- Perigrafi: Ena global Binary Max Heap.
- Skopos: Apothikeuei deiktes se vivlia, me proteraiotita vasi ths vathmologias (avg).
- Ylopoiisi:
  - Dynamikos pinakas apo deiktes se vivlia.
  - Ypostirizei insert, extract-max, kai update-key se O(log n).
  - Simantiko: Prostethike to pedio 'heap_pos' sto struct 'book_t' gia na 
    epitrepei O(1) evresi ths thesis enos vivliou mesa sto heap (gia grigora updates).
  - Xrisimopoieitai sta events: 'TOP' (Display top-k books) kai enimerwnetai 
    kata to 'R' (Return).

C. MemberActivity List (Linked List)
------------------------------------
- Perigrafi: Mia global, mi-taksinomimeni, monosyndedemeni lista.
- Skopos: Katagrafei statistika gia kathe melos (synolo daneismwn, reviews, score sum).
- Ylopoiisi:
  - Oi komvoi dimiourgountai/enimerwnontai se O(1) kata ta events Loan (L) kai Return (R).
  - Xrisimopoieitai sto event: 'AM' (Active Members).

=============================================================================
4. NEA EVENTS & ENTOLES (PHASE 2)
=============================================================================

1. F <title>: 
   Psaxnei gia ena vivlio me vasi ton titlo se ola ta genres xrisimopoiwntas ta AVL Trees.
   Poluplokotita: O(G * log n), opou G einai o arithmos twn genres.

2. TOP <k>: 
   Emfanizei ta 'k' vivlia me thn kalyteri vathmologia xrisimopoiwntas to Max Heap.
   Logiki: Kanei extract ta 'k' stoixeia gia na ta typwsei kai meta ta ksana-bazei 
   sto heap gia na diatirithei h domh.

3. AM: 
   Typwnei statistika (daneismous, reviews, meso oro vathmologias) gia ola ta 
   energa meli xrisimopoiwntas th lista MemberActivity.

4. U <bid> "<new_title>": 
   Enimerwnei ton titlo enos vivliou.
   Logiki: Diagrafei thn palia eggrafi apo to AVL -> Allazei to string tou titlou 
   -> Eisagei nea eggrafi sto AVL (me ton neo titlo). Elegxei gia monadikotita titlou.

5. X: 
   Typwnei ta synolika statistika tou systimatos (synolo vivliwn, melwn, energwn 
   daneismwn, global average score).

6. BF: 
   Eleutherwnei oli th desmeumeni mnimi (Genres, Books, Members, Loans, AVL Trees, 
   Heap, Activity List) kai termatizei to programma asfalws.

=============================================================================
5. DIAXEIRISH MNHMHS
=============================================================================
H synartisi 'free_all()' exei enimerwthei wste na eleutherwnei anadromika tis 
nees domes:
- free_book_index(): Eleutherwnei anadromika tous komvous tou AVL.
- free_recommendation_heap(): Eleutherwnei ton pinaka kai to struct tou heap.
- Eleutherwnei thn MemberActivity linked list.
To programma trexei xwris memory leaks.
