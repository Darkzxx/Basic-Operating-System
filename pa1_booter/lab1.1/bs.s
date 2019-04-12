        BOOTSEG =  0x9000        ! Boot block is loaded again to here.
        SSP      =   8192        ! Stack pointer at SS+8KB
	
        .globl _main                      ! IMPORT symbols
        .globl _getc,_putc                ! EXPORT symbols
	                                                
        !-------------------------------------------------------
        ! Only one SECTOR loaded at (0000,7C00). Get entire BLOCK in
        !-------------------------------------------------------
        mov  ax,#BOOTSEG    ! set ES to 0x9000
        mov  es,ax
        xor  bx,bx          ! clear BX = 0

        !---------------------------------------------------
        !  read boot BLOCK to [0x9000,0]     
        !---------------------------------------------------
        xor  dx,dx          ! drive 0, head 0
        xor  cx,cx          ! cyl 0    sector 0        
        incb cl             ! cyl 0, sector 1
        mov  ax, #0x0202    ! READ 1 block
        int  0x13

        jmpi    start,BOOTSEG           ! CS=BOOTSEG, IP=start

start:                    
        mov     ax,cs                   ! Set segment registers for CPU
        mov     ds,ax                   ! we know ES,CS=0x9000. Let DS=CS  
        mov     ss,ax                   ! SS = CS ===> all point at 0x9000
        mov     es,ax
        mov     sp,#SSP                 ! SP = 8KB above SS=0x9000

        mov     ax,#0x0012              ! 640x480 color     
	int     0x10 
	
        call _main                      ! call main() in C

        jmpi 0,0x1000
 

!======================== I/O functions =================================
		
        !---------------------------------------------
        !  char getc()   function: returns a char
        !---------------------------------------------
_getc:
        xorb   ah,ah           ! clear ah
        int    0x16            ! call BIOS to get a char in AX
        ret 

        !----------------------------------------------
        ! void putc(char c)  function: print a char
        !----------------------------------------------
_putc:           
        push   bp
	mov    bp,sp
	
        movb   al,4[bp]        ! get the char into aL
        movb   ah,#14          ! aH = 14
        movb   bl,#0x0D        ! bL = cyan color 
        int    0x10            ! call BIOS to display the char

        pop    bp
	ret
