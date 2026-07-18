C     Last change:  BCM  15 Jan 98   11:08 am
**==putbak.f    processed by SPAG 4.03F  at 09:52 on  1 Mar 1994
      SUBROUTINE putbak(Lstchr)
      IMPLICIT NONE
c     -----------------------------------------------------------------
      INCLUDE 'lex.i'
      CHARACTER Lstchr*(1)
      LOGICAL rngbuf
      EXTERNAL rngbuf
c     -----------------------------------------------------------------
      IF(Pos(PCHAR).le.1)THEN
       IF(rngbuf(3,Pos(PLINE),Linex,Lineln))THEN
        Pos(PCHAR)=Lineln
       ELSE
        CALL inpter(PERROR,Pos,'Can''t push input buffer back anymore')
       END IF
      END IF
c     -----------------------------------------------------------------
      IF(Lstchr.ne.Linex(Pos(PCHAR)-1:Pos(PCHAR)-1))THEN
       Pos(PCHAR)=Pos(PCHAR)-1
       CALL inpter(PERROR,Pos,
     &             '"'//Lstchr//'" is not the last character ')
       CALL abend
       RETURN
      ELSE
       Pos(PCHAR)=Pos(PCHAR)-1
      END IF
c     -----------------------------------------------------------------
      RETURN
      END
