/*  
 * mm.c 
 *  Our design is to maintain a binary search tree to manage the free blocks with different sizes. 
 *	We use a free block itself to be the tree node.  
 *  Every node in tree is also the head of a doubly linked list where all node are of same size. 
 *  Node in free_tree has four attributes left-child, right-child, parent, and brothers. 
 *  And every attribute is use a word size space in free block to perform.
 
 *	And if the node is in linked list which is node a tree node, 
 *	we use left-child to save the address of previous free block in this list, 
 *	and use brother to save the address of next free block in this list,
 *	also assign the right-child to -1 which differ from the tree node which do not have a right-child(on this case, assign it to 0).
 *
 *	Besides, it is wrapped by a 4 bytes header and a 4 bytes footer, which are used for coalescing
 *	
 *	The minimum size of a free block is 24 bytes.  
 *	When the size we need to allocate is no bigger than 512, we allocate a space with size of upper bond in (2^n)+4+4. 
 *	For example, when we need 10 bytes space, the upper bound in (2^n)+4+4 is (2^4)+4+4=24. 
 *	When the size we need is bigger than 512, we allocate a space with the size we need and other 8 bytes for pointers.
 *
 *  We control the check program by two variables: showflag and checkflag. 
 *	Only when these two are set to 1, the check few program can operate. 
 *	So we don't need to put them into comments.
 *
 *	Team member:
 *	Xiangtai Hou, netID: xhb083
 *	Haomin Zeng, netID: hzy075
 * 
 *
 *  Free BLOCK
 *        bp
 *        |
 *  ======================================================
 * | HEAD | LEFT | RIGHT | PART | BROS |            | FOOT |
 *  ======================================================
 *         (prev)  (-1)          (next)          ---------- when it is in the doubly linked list headed by tree node
 *
 *  ALLOCATED BLOCK 
 *        bp
 *        |
 *  ======================================================
 * | HEAD |   PAYLOAD                                | FOOT |
 *  ======================================================
 *
 */ 
