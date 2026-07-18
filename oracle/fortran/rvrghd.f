C     Last change:  BCM  15 Jan 98   11:54 am
      SUBROUTINE rvrghd(Othndl,Mt1,Lsav,Lprt)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Print out header information for revisions regressor history 
c     table      
c-----------------------------------------------------------------------
      INCLUDE 'title.cmn'
      INCLUDE 'revtbl.i'
      INCLUDE 'cchars.i'
c-----------------------------------------------------------------------
      INTEGER Othndl,Mt1
      LOGICAL Lsav,Lprt,locok
c-----------------------------------------------------------------------
      IF(Lsav)THEN
       CALL opnfil(.true.,.false.,LREVOT,Othndl,locok)
       IF(.not.locok)THEN
        CALL abend
        RETURN
       END IF
       WRITE(Othndl,1010)'date',TABCHR,'action',TABCHR,'regressors'
       WRITE(Othndl,1010)'----',TABCHR,'------',TABCHR,'----------'
      END IF
      IF(Lprt)THEN
       IF(Lpage)THEN
        WRITE(Mt1,Ttlfmt)Newpg,Title(1:Ntitle),Kpage,Serno(1:Nser)
        Kpage=Kpage+1
       END IF
       WRITE(Mt1,1020)
      END IF
c-----------------------------------------------------------------------
      RETURN
 1010 FORMAT(a,a,a,a,a)
 1020 FORMAT(//,' Actions on regARIMA outlier regressors from full ',
     &          'data span',//,
     &       4x,'Ending Date',5x,'Action',9x,'Outliers',/,
     &       4x,'-----------',5x,'------',9x,'--------')
      END
