pipeline {
    agent any    

    triggers {
        GenericTrigger(            

            causeString: 'Triggered by $name',

            token: '1111',
            tokenCredentialId: '',

            printContributedVariables: false,
            printPostContent: false,

            silentResponse: false,
        )
    }

    stages {        
        stage('Install push1st from apt repository.') {
            steps {                
		        echo 'Import repository key'
                sh 'wget https://nexus.naveksoft.com/repository/gpg/naveksoft.gpg.key -O naveksoft.gpg.key'
                sh 'sudo apt-key add naveksoft.gpg.key'
                echo 'Add repository to source list'
                sh 'echo "deb [arch=amd64] https://nexus.naveksoft.com/repository/ubuntu-universe/ universe main" | sudo tee /etc/apt/sources.list.d/naveksoft-universe.list'
                echo 'Check available versions'
                sh 'sudo apt update && apt list -a push1st'
                echo 'Install push1st from repository'
                sh 'sudo apt install -y push1st'
            }
        }

        stage('Run tests') {
            steps {
                sh 'sudo apt install -y python3-venv'
                sh 'pwd && ls -al'
                sh 'python3 -m venv special_env'
                sh 'ls -al'
                sh 'source special_env/bin/activate'
                echo 'Clone repo with tests'
                sh 'git clone git@bitbucket.org:naveksoft/functional-tests-pusher.git'
                echo 'Install requirements'
                sh 'cd functional-tests-pusher && pip3 install -r requirements.txt'
                echo 'Launch tests'
                sh 'cd functional-tests-pusher && bash launch_functional_tests_ps.sh'
                echo 'Print test_suite.log'
                sh 'cd functional-tests-pusher && cat test_suite.log'
                sh 'deactivate'
            }
        }       
    }

    post {
        cleanup { 
            cleanWs()
        }
    }    
}
