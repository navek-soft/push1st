pipeline {
    agent { label 'runner1' }   

    triggers {
        GenericTrigger(
            genericVariables: [
                [key: 'NAME', value: '$.commits[0].author.name'],
                [key: 'BRANCH', value: '$.ref']
            ],

            causeString: 'Triggered by $NAME, branch to build $BRANCH.',

            token: 'jenkins-build-push1st',
            tokenCredentialId: '',

            printContributedVariables: false,
            printPostContent: false,

            silentResponse: false,

            regexpFilterText: '$BRANCH_NAME',
            regexpFilterExpression: 'refactor-pipeline'
        )
    }

    environment {
        NEXUS = credentials('nexus-credentials')
        VERSION = "${AIPIX_MAJOR_VERSION}.${AIPIX_MINOR_VERSION}"
        SLACK_TOKEN = credentials('slack-oauth-token')
    }

    stages {
        stage('Build push1st image & push to nexus') {
            steps {
                sh """
                    git clone https://github.com/navek-soft/push1st.git
                    docker build --build-arg BUILD_NUMBER=${BUILD_NUMBER} \
                                 --build-arg VERSION=${VERSION} \
                                 --build-arg BRAND=aivp \
                                 -t download.aivp.io:8443/push1st/release:latest \
                                 -t download.aivp.io:8443/push1st/release:${VERSION} \
                                 -f ./docker/Dockerfile .
                    docker build --build-arg BUILD_NUMBER=${BUILD_NUMBER} \
                                 --build-arg VERSION=${VERSION} \
                                 --build-arg BRAND=aipix \
                                 -t download.aipix.ai:8443/push1st/release:latest \
                                 -t download.aipix.ai:8443/push1st/release:${VERSION} \
                                 -f ./docker/Dockerfile .
                    echo ${NEXUS_PSW} | docker login -u ${NEXUS_USR} --password-stdin https://download.aivp.io:8443
                    docker push download.aivp.io:8443/push1st/release:latest       
                    docker push download.aivp.io:8443/push1st/release:${VERSION}       
                    echo ${NEXUS_PSW} | docker login -u ${NEXUS_USR} --password-stdin https://download.aipix.ai:8443
                    docker push download.aipix.ai:8443/push1st/release:latest       
                    docker push download.aipix.ai:8443/push1st/release:${VERSION}       
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