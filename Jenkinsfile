pipeline {
    agent { label 'runner1' }   

    triggers {
        GenericTrigger(
            genericVariables: [
                [key: 'NAME', value: '$.actor.display_name'],
                [key: 'BRANCH', value: '$.push.changes[0].new.name']
            ],

            causeString: 'Triggered by $NAME, branch to build $BRANCH.',

            token: 'jenkins-build-push1st',
            tokenCredentialId: '',

            printContributedVariables: false,
            printPostContent: false,

            silentResponse: false,

            regexpFilterText: '$BRANCH',
            regexpFilterExpression: 'refactor-pipeline'
        )
    }

    environment {
        NEXUS = credentials('nexus-credentials')
        SLACK_TOKEN = credentials('slack-oauth-token')
    }

    stages {
        stage('Build push1st image & push to nexus') {
            when {
                expression { BRANCH ==~ /refactor-pipeline/ }
            }
            steps {
                // checkout([$class: 'GitSCM', 
                //           branches: [[name: '$BRANCH']], 
                //           extensions: [], 
                //           userRemoteConfigs: [[credentialsId: 'navek_jenkins-credentials', url: 'https://navek_jenkins@bitbucket.org/naveksoft/aipix-media-server.git']]
                //         ])
                sh """
                    ls -al                  
                """
            }
        }        
    }

    post {
        cleanup {
            cleanWs()
        }
    }
}