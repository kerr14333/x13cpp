      SUBROUTINE amdprt(Ardsp,Lprtmd,Prtbic)
      IMPLICIT NONE
c     ------------------------------------------------------------------
c     Provide reduced printout of ARIMA model parameters for automatic
c     model selection procedure
c     ------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'mdldat.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'lkhd.cmn'
      INCLUDE 'error.cmn'
c     ------------------------------------------------------------------
      INTEGER Ardsp,i,ipr,ips,idr,ids,iqr,iqs,id,ip,iq,iprs,iqrs,n
      LOGICAL Lprtmd,Prtbic
c     ------------------------------------------------------------------
c       Convert X-13A-S model variables into variables compatable with
c       TRAMO/SEATS model data structure
c     ------------------------------------------------------------------
      CALL cnvmdl(ipr,ips,idr,ids,iqr,iqs,id,ip,iq,iprs,iqrs,n)
      IF(Lfatal)RETURN
c     ------------------------------------------------------------------
      IF((ips+ids+iqs).gt.0)THEN
       IF(Lprtmd)THEN
        WRITE(Mt1,1010)ipr,idr,iqr,ips,ids,iqs
       ELSE
        WRITE(Mt1,1012)ipr,idr,iqr,ips,ids,iqs
*        WRITE(Mt1,1015)Armaer
        WRITE(Mt1,1030)
        RETURN
       END IF
      ELSE
       IF(Lprtmd)THEN
        WRITE(Mt1,1011)ipr,idr,iqr
       ELSE
        WRITE(Mt1,1013)ipr,idr,iqr
*        WRITE(Mt1,1015)Armaer
        WRITE(Mt1,1030)
        RETURN
       END IF
      END IF
c     ------------------------------------------------------------------
      IF(ipr.gt.0)WRITE(Mt1,1020)'              Regular AR : ',
     &                 (Arimap(Ardsp+i),i=1,ipr)
      IF(ips.gt.0)WRITE(Mt1,1020)'             Seasonal AR : ',
     &                 (Arimap(Ardsp+ipr+i),i=1,ips)
      IF(iqr.gt.0)WRITE(Mt1,1020)'              Regular MA : ',
     &                 (Arimap(Ardsp+iprs+i),i=1,iqr)
      IF(iqs.gt.0)WRITE(Mt1,1020)'             Seasonal MA : ',
     &                 (Arimap(Ardsp+iprs+iqr+i),i=1,iqs)
      IF(Prtbic)THEN
       WRITE(Mt1,1020)  '                     BIC : ',Bic
       WRITE(Mt1,1020)  '                    BIC2 : ',Bic2
      END IF
      WRITE(Mt1,1030)
c     ------------------------------------------------------------------
 1010 FORMAT('   Model Estimated : (',3(i2,1x),') (',3(i2,1x),')')
 1011 FORMAT('   Model Estimated : (',3(i2,1x),')')
 1012 FORMAT('   Estimation errors cause model (',3(i2,1x),
     &       ') (',3(i2,1x),') to be skipped')
 1013 FORMAT('   Estimation errors cause model (',3(i2,1x),
     &       ') to be skipped')
* 1015 FORMAT('     Armaer = ',i3)
 1020 FORMAT(a,4f10.4)
 1030 FORMAT('  -----')
c     ------------------------------------------------------------------
      RETURN
      END
