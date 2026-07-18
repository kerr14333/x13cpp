C     Last change:  BCM  17 Apr 2003   11:24 pm
      SUBROUTINE svf2f3(Nw,Ng,Lf2,Lf3,Arglab)
      IMPLICIT NONE
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'svllog.prm'
      INCLUDE 'svllog.cmn'
      INCLUDE 'x11svl.i'
      INCLUDE 'cmpsvl.i'
      INCLUDE 'inpt2.cmn'
      INCLUDE 'x11opt.cmn'
      INCLUDE 'work2.cmn'
      INCLUDE 'tests.cmn'
c-----------------------------------------------------------------------
c     Arglab is i if indirect adjustment, blank otherwise
c-----------------------------------------------------------------------
      CHARACTER Arglab*(*),yn*3,clbl*25
      INTEGER i,nlbl,n,Nw,Ng,im1
      LOGICAL Lf2,Lf3
c-----------------------------------------------------------------------
      DIMENSION yn(2)
c-----------------------------------------------------------------------
      DATA yn/'yes','no '/
c-----------------------------------------------------------------------
c     Print out X-11 diagnostics for F2
c-----------------------------------------------------------------------
      IF(Lf2)THEN
       DO i=1,Ny
        WRITE(Nw,1010)Arglab,i,Obar(i),Cibar(i),Ibar(i),Cbar(i),Sbar(i),
     &                Pbar(i),Tdbar(i),Smbar(i),Ombar(i),Cimbar(i),
     &                Imbar(i)
 1010   FORMAT(a,'2.a',i2.2,':',1x,E15.8,10(1X,E15.8))
       END DO
       DO i=1,Ny
        WRITE(Nw,1020)Arglab,i,Isq(i),Csq(i),Ssq(i),Psq(i),Tdsq(i),
     &                Osq2(i)
 1020   FORMAT(a,'2.b',i2.2,':',1x,5(2PF8.2),'  100.00',2PF8.2)
       END DO
       DO i=1,Ny
        WRITE(Nw,1030)Arglab,i,Obar2(i),Osd(i),Ibar2(i),Isd(i),Cbar2(i),
     &                Csd(i),Sbar2(i),Ssd(i),Cibar2(i),Cisd(i),Smbar2(i)
     &                ,Smsd(i)
 1030   FORMAT(a,'2.c',i2.2,':',12(1x,E15.8))
       END DO
       WRITE(Nw,1040)Arglab,Adrci,Adri,Adrc,Adrmcd
 1040  FORMAT(a,'2.d:',4F8.2)
       WRITE(Nw,1050)Arglab,(Smic(i),i=1,Ny)
 1050  FORMAT(a,'2.e:',12F8.2)
       WRITE(Nw,1060)Arglab,Mcd
 1060  FORMAT(a,'2.mcd:',i8)
       WRITE(Nw,1070)Arglab,Vi,Vc,Vs,Vp,Vtd,Rv
 1070  FORMAT(a,'2.f:',6F8.2)
       n=Ny+2
       WRITE(Nw,1080)Arglab,(Autoc(i),i=1,n)
 1080  FORMAT(a,'2.g:',14F8.2)
       WRITE(Nw,1090)Arglab,Ratic,Arglab,Ratis
 1090  FORMAT(a,'2.ic:',F12.2,/,a,'2.is:',F12.2)
       WRITE(Nw,1100)Arglab,Fpres,P3
 1100  FORMAT(a,'2.fsb1:',F11.3,F8.2)
       WRITE(Nw,1120)Arglab,Fstabl,P1,Arglab,Chikw,P5,Arglab,Fmove,P2
 1120  FORMAT(a,'2.fsd8:',F11.3,F8.2,/,a,'2.kw:',F11.3,F8.2,/,
     &        a,'2.msf:',F11.3,F8.2)
       WRITE(Nw,1121)Arglab,yn(Iqfail)
 1121  FORMAT(a,'2.idseasonal: ',a)
      END IF
c-----------------------------------------------------------------------
      IF(Arglab(1:1).eq.'i')THEN
       clbl=' (indirect adjustment) : '
       nlbl=25
       IF(Svltab(LSLISR).and.Kfulsm.lt.2)
     &    WRITE(Ng,2010)clbl(1:nlbl),Ratis
       IF(Svltab(LSLIIR))WRITE(Ng,2020)clbl(1:nlbl),Ratic
       IF(Svltab(LSLID8))WRITE(Ng,2030)clbl(1:nlbl),Fstabl
       IF(Svltab(LSLISF))WRITE(Ng,2040)clbl(1:nlbl),Fmove
       IF(Svltab(LSLIID))WRITE(Ng,2060)clbl(1:nlbl),yn(Iqfail)
       im1=LSLIM1
      ELSE
       clbl=' : '
       nlbl=3
       IF(Svltab(LSLMSR).and.Kfulsm.lt.2)
     &    WRITE(Ng,2010)clbl(1:nlbl),Ratis
       IF(Svltab(LSLICR))WRITE(Ng,2020)clbl(1:nlbl),Ratic
       IF(Svltab(LSLFB1))WRITE(Ng,2050)clbl(1:nlbl),Fpres
       IF(Svltab(LSLFD8))WRITE(Ng,2030)clbl(1:nlbl),Fstabl
       IF(Svltab(LSLMSF))WRITE(Ng,2040)clbl(1:nlbl),Fmove
       IF(Svltab(LSLIDS))WRITE(Ng,2060)clbl(1:nlbl),yn(Iqfail)
       im1=LSLM1
      END IF
 2010 FORMAT('  Moving seasonality ratio    ',a,f11.3)
 2020 FORMAT('  I/C Ratio                   ',a,f11.3)
 2030 FORMAT('  Stable Seasonal F, D8 table ',a,f11.3)
 2040 FORMAT('  Moving Seasonal F, D8 table ',a,f11.3)
 2050 FORMAT('  Stable Seasonal F, B1 table ',a,f11.3)
 2060 FORMAT('  Identifiable seasonality    ',a,a)
c-----------------------------------------------------------------------
c     Print out Quality control diagnostics for F3
c-----------------------------------------------------------------------
      DO i=1,Nn
       IF(i.ne.6.or.Kfulsm.lt.2)THEN
        IF(Lf3)WRITE(Nw,1130)Arglab,i,Qu(i)
 1130   FORMAT(a,'3.m',i2.2,':',1x,f6.3)
        IF(Svltab(im1+i-1))WRITE(Ng,2070)i,clbl(1:nlbl),Qu(i)
 2070   FORMAT('    M',i2.2,a,f10.3)
       END IF
      END DO
      IF(Lf3)THEN
       WRITE(Nw,1140)Arglab,Qual,Arglab,Q2m2,Arglab,Kfail
 1140  FORMAT(a,'3.q:',1x,F5.2,/,a,'3.qm2:',1x,F5.2,/,a,'3.fail:',
     &        1x,i2)
      END IF
      IF(Svltab(im1+11))WRITE(Ng,2080)'    Q  ',clbl(1:nlbl),Qual
      IF(Svltab(im1+12))WRITE(Ng,2080)'    Q2 ',clbl(1:nlbl),Q2m2
 2080 FORMAT(a,a,f10.3)
      RETURN
      END
