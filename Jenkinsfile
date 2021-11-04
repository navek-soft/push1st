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
        stage('Installation from apt repository') {
            steps {                
                echo 'Import repository key'
                sh 'wget https://reader:reader1@nexus.naveksoft.com/repository/gpg/naveksoft.gpg.key -O naveksoft.gpg.key'
                sh 'sudo apt-key add naveksoft.gpg.key'
                echo 'Add repository to source list and adjust auth'
                sh 'echo "deb [arch=amd64] https://nexus.naveksoft.com/repository/ubuntu-universe/ universe main" | sudo tee /etc/apt/sources.list.d/naveksoft-universe.list'
                sh 'echo "machine nexus.naveksoft.com/repository login reader password reader1" | sudo tee /etc/apt/auth.conf.d/nexus.naveksoft.com.conf'
                echo 'Check available versions'
                sh 'sudo apt update && apt list -a push1st'
                echo 'Install from repository'
                sh 'sudo apt install -y push1st'                            	   
            }
        }

        stage('Run tests') {
            steps {
                sh 'sudo apt-get install python3-venv'
                sh 'python3 -m venv special_env'
                sh 'source special_env/bin/activate'
                echo 'Clone repo with tests'
                sh 'git clone git@bitbucket.org:naveksoft/functional-tests-pusher.git'
                echo 'Install requirements'
                sh 'cd functional-tests-pusher && pip3 install -r requirements.txt'
                echo 'Launch tests'
                sh 'cd functional-tests-pusher && bash launch_functional_tests_ps.sh'
                echo 'test_suite.log'
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
