      SUBROUTINE prtukp(Mt,Iagr,Ny,Lsvlg)
      IMPLICIT NONE
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'arima.cmn'
      INCLUDE 'title.cmn'
      INCLUDE 'tukey.cmn'
      INCLUDE 'rho.cmn'
      INCLUDE 'error.cmn'
      INCLUDE 'spctbl.i'
c-----------------------------------------------------------------------
      LOGICAL Lsvlg
      CHARACTER thisLb*(36),cPeak*(2),begstr*(10),endstr*(10)
      INTEGER Mt,Iagr,i,k,nLb,nchr1,nchr2,Ny
      DOUBLE PRECISION thisPk,thisTd
      DIMENSION thisPk(6),cPeak(7)
c-----------------------------------------------------------------------
      IF(Lpage.and.(.not.Lsvlg))THEN
       WRITE(Mt,Ttlfmt)Newpg,Title(1:Ntitle),Kpage,Serno(1:Nser)
       Kpage=Kpage+1
      END IF
      IF(Iagr.eq.4)THEN
       WRITE(Mt,1000)'  Peak probabilities for Tukey spectrum '//
     &                'estimator: Indirect adjustments'
      ELSE
       WRITE(Mt,1000)
     &    '  Peak probabilities for Tukey spectrum estimator'
      END IF
c-----------------------------------------------------------------------
      CALL wrtdat(Bgspec,Ny,begstr,nchr1)
      IF(.not.Lfatal)CALL wrtdat(Endspn,Ny,endstr,nchr2)
      IF(Lfatal)RETURN
      WRITE(Mt,1020)begstr(1:nchr1),endstr(1:nchr2)
      WRiTE(Mt,1005)
c-----------------------------------------------------------------------
      DO i=1,Ntukey
       thisLb=' '
c-----------------------------------------------------------------------
c   Set up labels, peak vectors
c-----------------------------------------------------------------------
       IF(Itukey(i).eq.LSPCRS)THEN
        CALL copy(Ptsr,6,1,thisPk)
        thisTD=Pttdr
        nLb=16
        thisLb(1:nLb)=' Model Residuals'
       ELSE IF(Itukey(i).eq.LSPTS0.or.Itukey(i).eq.LSPT0C)THEN
        CALL copy(Ptso,6,1,thisPk)
        thisTD=Pttdo
        CALL mkspst(Spcsrs,thisLb,nLb,k,.true.)
       ELSE IF(Itukey(i).eq.LSPTS1.or.Itukey(i).eq.LSPT1I.or.
     &         Itukey(i).eq.LSPT1S)THEN
        CALL copy(Ptsa,6,1,thisPk)
        thisTD=Pttda
        IF(Itukey(i).eq.LSPTS1)THEN
         IF(Lrbstsa)THEN
          nLb=32
          thisLb(1:nLb)=' Seasonally adjusted series (E2)'
         ELSE
          nLb=33
          thisLb(1:nLb)=' Seasonally adjusted series (D11)'
         END IF
        ELSE IF(Itukey(i).eq.LSPT1I)THEN
         IF(Lrbstsa)THEN
          nLb=33
          thisLb(1:nLb)=' Ind. Seasonally adj. series (E2)'
         ELSE
          nLb=34
          thisLb(1:nLb)=' Ind. Seasonally adj. series (D11)'
         END IF
        ELSE IF(Itukey(i).eq.LSPT1S)THEN
         nLb=35
         thisLb(1:nLb)=' Seasonally adjusted series (SEATS)'
        END IF
       ELSE IF(Itukey(i).eq.LSPTS2.or.Itukey(i).eq.LSPT2I.or.
     &         Itukey(i).eq.LSPT2S)THEN
        CALL copy(Ptsi,6,1,thisPk)
        thisTD=Pttdi
        IF(Itukey(i).eq.LSPTS2)THEN
         IF(Lrbstsa)THEN
          nLb=24
          thisLb(1:nLb)=' Modified Irregular (E3)'
         ELSE
          nLb=16
          thisLb(1:nLb)=' Irregular (D13)'
         END IF
        ELSE IF(Itukey(i).eq.LSPT2I)THEN
         IF(Lrbstsa)THEN
          nLb=33
          thisLb(1:nLb)=' Indirect Modified Irregular (E3)'
         ELSE
          nLb=25
          thisLb(1:nLb)=' Indirect Irregular (D13)'
         END IF
        ELSE IF(Itukey(i).eq.LSPT2S)THEN
         IF(Lrbstsa)THEN
          nLb=29
          thisLb(1:nLb)=' Stochastic Irregular (SEATS)'
         ELSE
          nLb=18
          thisLb(1:nLb)=' Irregular (SEATS)'
         END IF
        END IF
       END IF
c-----------------------------------------------------------------------
c   Set up 
c-----------------------------------------------------------------------
       DO k=1,6
        IF(thisPk(k).gt.0.99D0)THEN
         cPeak(k)='**'
        ELSE IF(thisPk(k).gt.0.90D0)THEN
         cPeak(k)='* '
        ELSE
         cPeak(k)='  '
        END IF
       END DO
       IF(thisTD.gt.0.99D0)THEN
        cPeak(7)='**'
       ELSE IF(thisTD.gt.0.90D0)THEN
        cPeak(7)='* '
       ELSE
        cPeak(7)='  '
       END IF
c-----------------------------------------------------------------------
c    Write out probabilities and peak labels for each seasonal freq,
c    trading day
c-----------------------------------------------------------------------
       IF(Lsvlg)then
        WRITE(Mt,1030)thisLb,(cPeak(k),k=1,6),cPeak(7)
       ELSE
        WRITE(Mt,1010)thisLb,(thisPk(k),cPeak(k),k=1,6),thisTD,cPeak(7)
*        WRITE(Mt,1010)thisLb,(thisPk(k),k=1,6),thisTD
*        WRITE(Mt,1020)(cPeak(k),k=1,6),cPeak(7)
       END IF
      END DO
      WRITE(Mt,1040)
c-----------------------------------------------------------------------
 1000 FORMAT(/,a)
 1005 FORMAT(40x,
     &   '  S1       S2       S3       S4       S5       S6       TD',
     &     /,40x, 
     &   '------   ------   ------   ------   ------   ------   ------')
* 1000 FORMAT(a,/,40x,'  S1    S2    S3    S4    S5    S6    TD',/,
*     &           40x,'----- ----- ----- ----- ----- ----- -----')
 1010 FORMAT(1x,A36,3x,7(F6.3,A2,1x))
 1020 FORMAT('  Spectrum estimated from ',a,' to ',a,'.',/)
* 1010 FORMAT(1x,A36,3x,7(F5.3,1x))
* 1020 FORMAT(40x,7(2x,a2,2x))
 1030 FORMAT(1x,A36,3x,7(3x,a2,4x))
 1040 FORMAT('  ----------',/,
     &       5x,'** - Peak Probability > 0.99,',/,
     &       5x,' * - 0.90 < Peak Probability < 0.99',//)
c-----------------------------------------------------------------------
      RETURN
      END
