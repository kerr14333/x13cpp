      SUBROUTINE prtcol(L,Nline,Tblcol,Tblwid,Ny,Mt1,Nop,Noplbl,Disp2,
     &                  Disp3,Fmtcol,Colhdr)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     This subroutine prints column headers for the table subroutine.
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
c-----------------------------------------------------------------------
      CHARACTER blnk*22,dash*132,Colhdr*22,Noplbl*5,thdr*22
      CHARACTER Fmtcol*110
      INTEGER i,ijk,ihdr,jhdr,khdr,ndash,L,Tblcol,Tblwid,Mt1,Nop,Nline,
     &        Ny,Disp2,Disp3
      DIMENSION Colhdr(PSP+2),thdr(PSP+2) 
c-----------------------------------------------------------------------
c     Initialize variables
c-----------------------------------------------------------------------
      CALL setchr(' ',22,blnk)
      CALL setchr('-',132,dash)
      dash(1:1)=' '
c-----------------------------------------------------------------------
c     Write dashes over column headers
c-----------------------------------------------------------------------
      ndash=Tblcol*(Tblwid+Disp2)+10
      IF(Nop.lt.5)THEN
       IF(Disp3.eq.35)THEN
        ndash=ndash+Tblwid+5
       ELSE IF(Disp3.eq.56)THEN
        ndash=ndash+Tblwid+6
       ELSE
        ndash=ndash+Tblwid+2+Disp3
       END IF
      END IF
      WRITE(Mt1,1010)dash(1:ndash)
c-----------------------------------------------------------------------
c     Set number of elements in Colhdr.  Nline determines if the full
c     header is to be printed out (Nline=0) or some part of it.
c-----------------------------------------------------------------------
      IF(Nline.eq.0)THEN
       ihdr=L+1
       jhdr=L
       khdr=0
      ELSE
       ihdr=Tblcol+2
       jhdr=Tblcol+1
       khdr=(Nline-1)*Tblcol
       IF((ihdr+khdr).gt.L)THEN
        ihdr=Ny-khdr+2
        jhdr=Ny-khdr+1
       END IF
      END IF
      IF(Nop.eq.5)ihdr=ihdr-1
c-----------------------------------------------------------------------
c     Set up headers for columns
c-----------------------------------------------------------------------
      thdr(1)=Colhdr(1)
      DO ijk=2,jhdr
       thdr(ijk)=Colhdr(ijk+khdr)
      END DO
c-----------------------------------------------------------------------
c     If a column of summary data is included with the table, enter the 
c     label for that column at the end of Colhdr.
c-----------------------------------------------------------------------
      IF(ihdr.gt.jhdr)THEN
       thdr(ihdr)=blnk(1:22)
       thdr(ihdr)((Tblwid-4):Tblwid)=Noplbl
      END IF
c-----------------------------------------------------------------------
c     Print out column header
c-----------------------------------------------------------------------
      WRITE(Mt1,Fmtcol)(thdr(i),i=1,ihdr)
c-----------------------------------------------------------------------
c     Print out dashes after column header
c-----------------------------------------------------------------------
      WRITE(Mt1,1010)dash(1:ndash)
 1010 FORMAT(a)
c-----------------------------------------------------------------------
      RETURN
      END
