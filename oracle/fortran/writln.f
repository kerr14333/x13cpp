C     Last change:  BCM  13 Oct 1998   11:09 am
**==writln.f    processed by SPAG 4.03F  at 09:55 on  1 Mar 1994
      SUBROUTINE writln(Oline,Flhdnl,Flhdn2,Lblnk)
      IMPLICIT NONE
c     -----------------------------------------------------------------
      INCLUDE 'units.cmn'
*      INCLUDE 'error.cmn'
c     -----------------------------------------------------------------
      INTEGER Flhdnl,Flhdn2
      CHARACTER Oline*(*)
      LOGICAL Lblnk
c     -----------------------------------------------------------------
      IF(Flhdnl.eq.Mt2.or.Flhdn2.eq.Mt2)CALL errhdr
      IF(Flhdnl.gt.0)THEN
       IF(Lblnk)WRITE(Flhdnl,1010)' '
       WRITE(Flhdnl,1010)Oline(1:min(131,len(Oline)))
      END IF
      IF(Flhdn2.gt.0)THEN
       IF(Lblnk)WRITE(Flhdn2,1010)' '
       WRITE(Flhdn2,1010)Oline(1:min(131,len(Oline)))
      END IF
 1010 FORMAT(' ',a)
c     -----------------------------------------------------------------
      RETURN
      END
