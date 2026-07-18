**==skpfcn.f    processed by SPAG 4.03F  at 09:53 on  1 Mar 1994
      SUBROUTINE skpfcn(Fname,Nfn)
c-----------------------------------------------------------------------
c     Trys to point the input stream beyond the current specification
c-----------------------------------------------------------------------
      IMPLICIT NONE
      INCLUDE 'lex.i'
c     ------------------------------------------------------------------
      CHARACTER Fname*(LINLEN)
      INTEGER Nfn
c     ------------------------------------------------------------------
      DO WHILE (.true.)
       IF(Nxtktp.eq.EOF)THEN
        CALL inpter(PERROR,Lstpos,
     &              'No closing brace "}" on specification, "'//
     &              Fname(1:Nfn)//'"')
c     ------------------------------------------------------------------
       ELSE IF(Nxtktp.ne.RBRACE)THEN
        CALL lex()
        GO TO 10
c     ------------------------------------------------------------------
       ELSE
        CALL lex()
       END IF
       GO TO 20
   10  CONTINUE
      END DO
c     ------------------------------------------------------------------
   20 RETURN
      END
