c-----------------------------------------------------------------------
c     ref_grpx.f -- what does editor.f:1786's Grpx(Tdgrp-1) READ when
c     Tdgrp==0?
c
c     Compiled against the VENDORED oracle headers, read-only. The claim
c     under test: COMMON /cx11rg/ declares Clxptr(0:PB) immediately before
c     Grpx(0:PGRP), so Fortran storage association makes Grpx(-1) resolve
c     to Clxptr(PB) -- a determined in-COMMON address, not a wild read.
c
c     Method: poison both arrays with distinguishable values (Clxptr(i) =
c     1000+i, Grpx(i) = 2000+i), then perform the same subscript
c     expression editor.f uses. 1080 proves the alias; 2000-ish or garbage
c     disproves it. The read is done inside a SUBROUTINE taking Tdgrp as an
c     argument so the compiler cannot constant-fold it at -O0.
c-----------------------------------------------------------------------
      PROGRAM refgrpx
      IMPLICIT NONE
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'xrgmdl.cmn'
      INTEGER i,begcol,endcol
c-----------------------------------------------------------------------
      DO i=0,PB
       Clxptr(i)=1000+i
      END DO
      DO i=0,PGRP
       Grpx(i)=2000+i
      END DO
      DO i=0,PGRP
       Gpxptr(i)=3000+i
      END DO
c-----------------------------------------------------------------------
      CALL rdgrp(0,begcol,endcol)
      WRITE(*,1000)'PB                 ',PB
      WRITE(*,1000)'PGRP               ',PGRP
      WRITE(*,1000)'Clxptr(PB)         ',Clxptr(PB)
      WRITE(*,1000)'Clxptr(PB-1)       ',Clxptr(PB-1)
      WRITE(*,1000)'Grpx(0)            ',Grpx(0)
      WRITE(*,1000)'begcol = Grpx(-1)  ',begcol
      WRITE(*,1000)'endcol = Grpx(0)-1 ',endcol
      WRITE(*,*)   'alias is Clxptr(PB)?',begcol.eq.Clxptr(PB)
      WRITE(*,*)   'flip (begcol.eq.endcol)?',begcol.eq.endcol
c-----------------------------------------------------------------------
c     Second probe: the values a REAL run would carry. loadxr.f:38 copies
c     all PB+1 elements of Colptr into Clxptr, so Clxptr(PB) is Colptr(PB)
c     whether or not the model has that many columns. Simulate a small
c     model over a zeroed block: Colptr(0)=1, a few used entries, the rest
c     left at 0 -- the state a static COMMON starts in.
c-----------------------------------------------------------------------
      DO i=0,PB
       Clxptr(i)=0
      END DO
      Clxptr(0)=1
      DO i=1,6
       Clxptr(i)=1+i*10
      END DO
      DO i=0,PGRP
       Grpx(i)=0
      END DO
      Grpx(0)=1
      CALL rdgrp(0,begcol,endcol)
      WRITE(*,*)   '--- zeroed-block simulation ---'
      WRITE(*,1000)'Clxptr(PB)         ',Clxptr(PB)
      WRITE(*,1000)'begcol = Grpx(-1)  ',begcol
      WRITE(*,1000)'endcol = Grpx(0)-1 ',endcol
      WRITE(*,*)   'flip (begcol.eq.endcol)?',begcol.eq.endcol
 1000 FORMAT(1x,a,' = ',i8)
      END

c-----------------------------------------------------------------------
      SUBROUTINE rdgrp(Tdgrp,Begcol,Endcol)
      IMPLICIT NONE
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'xrgmdl.cmn'
      INTEGER Tdgrp,Begcol,Endcol
c     the two lines from editor.f:1785-1786, verbatim
      Begcol=Grpx(Tdgrp-1)
      Endcol=Grpx(Tdgrp)-1
      RETURN
      END
