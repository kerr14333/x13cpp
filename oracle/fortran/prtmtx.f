C     Last change:  BCM  12 Mar 98   12:30 pm
      SUBROUTINE prtmtx(Begxy,Sp,Xy,Nrxy,Ncxy,Ttlstr,Ttlptr,Nttl)
      IMPLICIT NONE
c     ------------------------------------------------------------------
c      INCLUDE 'srslen.prm'
c      INCLUDE 'model.prm'
      INCLUDE 'units.cmn'
      INCLUDE 'title.cmn'
      INCLUDE 'error.cmn'
c     ------------------------------------------------------------------
      CHARACTER dash*25,space*25
      CHARACTER str*(10),Ttlstr*(*),fmt1*(7),fmt2*(9),fmt3*(21)
      INTEGER Begxy,ibeg,ielt,iend,idate,nchr,Ncxy,Nrxy,Nttl,Sp,Ttlptr,
     &        nt,colwid,maxwid,ncol
      INTEGER Mxtbwd
      DOUBLE PRECISION Xy
      DIMENSION Begxy(2),idate(2),Ttlptr(0:Nttl),Xy(*),colwid(11)
      DATA dash/'-------------------------'/
      DATA space/'                         '/
c     ------------------------------------------------------------------
c     Initialize variables used in printing matrix
c     Changed by Brian Monsell, October 25, 1994
c     ------------------------------------------------------------------
      Mxtbwd=80
      IF(Lwdprt)Mxtbwd=132
      CALL setint(11,11,colwid)
      maxwid=11
      DO ielt=1,Nttl
       maxwid=max(Ttlptr(ielt)-Ttlptr(ielt-1),maxwid)
      END DO
      maxwid=maxwid+2
      ncol=(Mxtbwd-10)/maxwid
      DO ielt=1,Nttl
       nt=mod(ielt,ncol)
       IF(nt.eq.0)nt=ncol
       colwid(nt)=max(Ttlptr(ielt)-Ttlptr(ielt-1),colwid(nt))
      END DO
      nt=min(Nttl,ncol)
      WRITE(fmt1,1010)'a',2*ncol
      WRITE(fmt2,1010)'t11',2*ncol
 1010 FORMAT('(',a,',',i2,'a)')
c     ------------------------------------------------------------------
      WRITE(Mt1,fmt1)'      Date',
     &               (space(1:max(maxwid+Ttlptr(ielt-1)-Ttlptr(ielt),1))
     &               ,Ttlstr(Ttlptr(ielt-1):Ttlptr(ielt)-1),ielt=1,nt)
      IF(Nttl.gt.ncol)WRITE(Mt1,fmt2)(space(1:max(maxwid+Ttlptr(ielt-1)-
     &                               Ttlptr(ielt),1)),
     &                               Ttlstr(Ttlptr(ielt-1):Ttlptr(ielt)
     &                               -1),ielt=nt+1,Nttl)
      WRITE(Mt1,fmt1)'      ----',
     &               (space(1:maxwid-colwid(ielt)),dash(1:colwid(ielt)),
     &               ielt=1,nt)
c     ------------------------------------------------------------------
      WRITE(fmt3,1020)ncol,maxwid
 1020 FORMAT('(2x,a8,(:t11,',i1,'E',i2,'.4))')
      DO iend=Nttl,Ncxy*Nrxy,Ncxy
       ibeg=iend-Nttl+1
       CALL addate(Begxy,Sp,(iend-Nttl+Ncxy)/Ncxy-1,idate)
       CALL wrtdat(idate,Sp,str,nchr)
       IF(Lfatal)RETURN
       WRITE(Mt1,fmt3)str(1:nchr),(Xy(ielt),ielt=ibeg,iend)
      END DO
c     ------------------------------------------------------------------
      RETURN
      END
