      SUBROUTINE amidot(A,Trnsrs,Frstry,Nefobs,Priadj,Convrg,Fctok,
     &                  Argok)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Run automatic outlier identification within the automatic model
c     identification procedure.  The routine was created to allow
c     automatic outlier identification to be run when the final tests
c     change the order of differencing in the final model without
c     duplicating a great deal of code.  
c     (BCM April 2007)
c-----------------------------------------------------------------------
      LOGICAL T,F
      PARAMETER(T=.true.,F=.false.)
c-----------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'arima.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'extend.cmn'
      INCLUDE 'error.cmn'
      INCLUDE 'tbllog.prm'
      INCLUDE 'tbllog.cmn'
      INCLUDE 'mdltbl.i'
c-----------------------------------------------------------------------
      DOUBLE PRECISION A,Trnsrs
      INTEGER Frstry,Nefobs,Priadj,nfil
      LOGICAL Argok,Convrg,Fctok
      DIMENSION A(PLEN+2*PORDER),Trnsrs(PLEN)      
c-----------------------------------------------------------------------
      INTEGER nblank
      EXTERNAL nblank
c-----------------------------------------------------------------------
c    Run automatic model identification procedure
c-----------------------------------------------------------------------
*      CALL idotlr(Ltstao,Ltstls,Ltsttc,Ltstso,Ladd1,Critvl,Cvrduc,
      CALL idotlr(Ltstao,Ltstls,Ltsttc,Ladd1,Critvl,Cvrduc,
     &            Begtst,Endtst,Nefobs,Lestim,Mxiter,Mxnlit,Argok,A,
     &            Trnsrs,Nobspf,Nfcst,Outfct,Fctok,F,0,F,Prttab(LAUOTT),
     &            Prttab(LAUOTI),Savtab(LAUOTI),Prttab(LAUOFT),
     &            Savtab(LAUOFT),F,F)
c-----------------------------------------------------------------------
c   If model doesn't converge somewhere in the automatic outlier
c   identification procedure, print a message 
c-----------------------------------------------------------------------
      IF((.not.Lfatal).and.(.not.Convrg))THEN
       IF(Prttab(LAUOTI))WRITE(Mt1,1010)MDLSEC,PRGNAM,DOCNAM
       IF(Prttab(LAUMCH))WRITE(Mt1,1020)
       Bstdsn(1:4)='none'
       Nbstds=4
       CALL abend()
       RETURN
      END IF
c-----------------------------------------------------------------------
c      print out more details if estimation error occurs in outlier
c      identification phase of the procedure
c      BCM February 2007
c-----------------------------------------------------------------------
      IF(.not.Argok)THEN
       nfil=nblank(Cursrs)
       CALL writln('ERROR: A model estimation error has occurred during 
     &outlier identification',STDERR,Mt1,T)
       CALL writln('       within the automatic model identification pro
     &cedure; for more details,',STDERR,Mt1,F)
       CALL writln('       check the error file ('//Cursrs(1:nfil)//
     &             ').',STDERR,Mt1,F)
       CALL abend()
      END IF
c-----------------------------------------------------------------------
c     Reform regression matrix
c-----------------------------------------------------------------------
      IF(.not.Lfatal)CALL regvar(Trnsrs,Nobspf,Fctdrp,Nfcst,0,Userx,
     &                           Bgusrx,Nrusrx,Priadj,Reglom,Nrxy,
     &                           Begxy,Frstry,T,Elong)
c-----------------------------------------------------------------------
 1010 FORMAT('        Rerun program trying one of the following:',/,
     &       '          (1) Allow more iterations (set a larger ',
     &       'value of maxiter).',/,
     &       '          (2) Lower one of the values of maxorder.',
     &       '        See ',a,' of the ',a,' ',a,' for more',/,
     &       ' discussion.')
 1020 FORMAT(/,3x,'No models have been selected due to errors in model',
     &         ' estimation.')
c-----------------------------------------------------------------------
      RETURN
      END
