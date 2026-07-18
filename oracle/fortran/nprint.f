      SUBROUTINE nprint(Fh,S)
c-----------------------------------------------------------------------
c     eprint.f, Release 1, Subroutine Version 1.3, Modified 24 Jan 1995.
c-----------------------------------------------------------------------
c     eprint - print error, Lahey pc version
c-----------------------------------------------------------------------
c   Author     - Larry Bobbitt
c                Statistical Research Division
c                U.S. Census Bureau
c                Room 3000-4
c                Washington, D.C.    20233
c                (301) 763-3957
c-----------------------------------------------------------------------
      IMPLICIT NONE
c     ------------------------------------------------------------------
      CHARACTER*(*) S
      INTEGER Fh
c     ------------------------------------------------------------------
      WRITE(Fh,*)' <p><strong>NOTE:</strong>  ',S,'</p>'
c     ------------------------------------------------------------------
      RETURN
      END
