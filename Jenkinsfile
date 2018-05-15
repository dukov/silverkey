pipeline {
  agent none

  environment {
    JOB_QT_APP = "silverkey-qt"
  }


  stages {
    stage('Build') {
      parallel {
        stage('linux') {
          environment {
            STAGE_ARCH = "x64_86"
            STAGE_OS = "linux"
            STAGE_ARTIFACT = "${JOB_QT_APP}-${STAGE_OS}-${STAGE_ARCH}"
          }
          agent {
            dockerfile {
              reuseNode true
              label 'master'
            }
          }
          steps {
            dir('src') {
              sh 'qmake'
              sh 'make -j4'
              sh "mv -v ${JOB_QT_APP} ${STAGE_ARTIFACT}"
            }
            archiveArtifacts "src/${STAGE_ARTIFACT}"
          }
        }
        stage('osx') {
          environment {
            STAGE_ARTIFACT = "${JOB_QT_APP}.app/**"
          }
          agent {
            label 'mac-slave'
          }
          steps {
            dir('src') {
              sh '/usr/local/Cellar/qt/5.10.1/bin/qmake'
              sh 'make -j4'
              sh '/usr/local/Cellar/qt/5.10.1/bin/macdeployqt ${JOB_QT_APP}.app'

              archiveArtifacts "${STAGE_ARTIFACT}"
            }
          }
        }
      }
    }
  }
  post {
    success {
      script {
        // CHANGE_ID is set only for pull requests, so it is safe to access the pullRequest global variable
        if (env.CHANGE_ID) {
          pullRequest.addLabel('Build success')
        }
      }
    }
  }
}