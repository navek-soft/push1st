// Api call to trigger BB-pipeline
// curl -X POST -is -u navek_jenkins:d8HD3xMVeRffqUR2UNn4 -H 'Content-Type: application/json' https://api.bitbucket.org/2.0/repositories/naveksoft/jenkins-push1st/pipelines/ -d '{"target": {"ref_type": "branch", "type": "pipeline_ref_target", "ref_name": "master", "selector": {"type": "custom", "pattern": "Push1st build image && push"}}}'

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
            regexpFilterExpression: 'main'
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
                                 --build-arg COMMIT=${COMMIT:-6ce8dcfac9a9b9bbd22a25f3b68752763f71aa21} \
                                 --platform linux/amd64,linux/arm64 \
                                 -t download.aivp.io:8443/push1st/release:latest \
                                 -t download.aivp.io:8443/push1st/release:${VERSION} \
                                 -f ./docker/Dockerfile .
                    docker build --build-arg BUILD_NUMBER=${BUILD_NUMBER} \
                                 --build-arg VERSION=${VERSION} \
                                 --build-arg BRAND=aipix \
                                 --build-arg COMMIT=${COMMIT:-6ce8dcfac9a9b9bbd22a25f3b68752763f71aa21} \
                                 --platform linux/amd64,linux/arm64 \
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