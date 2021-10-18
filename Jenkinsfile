pipeline {
    agent any

    environment {
        SLACK_TOKEN = credentials('41')
    }

    triggers {
        GenericTrigger(
            // genericVariables: [
            //     [key: 'name', value: '$.pusher.name'],                
            //     [key: 'url', value: '$.commits.[0].url'],
            //     [key: 'branch', value: '$.repository.master_branch'],
            // ],

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
                sh 'sudo apt update && sudo apt install push1st'                
                // sh 'curl -F file=@${WORKSPACE}/plan.txt \
                //     -F filetype=auto \
                //     -F "initial_comment=Terraform plan output:" \
                //     -F channels=C029JCMAG20 \
                //     -H "Authorization: Bearer $SLACK_TOKEN" \
                //     https://slack.com/api/files.upload'                	   
            }
        }

        stage('Run tests') {
            steps {
		        echo 'Clone repo with tests'
                sh 'git clone git@bitbucket.org:naveksoft/functional-tests-pusher.git'
                echo 'Install requirements'
                sh 'cd functional-tests-pusher && pip3 install -r requirements.txt'
                echo 'Launch tests'
                sh './launch_functional_tests_ps.sh'                   	   
            }
        }

        // stage('slack approve plan') {            
        //     steps {
        //         script {
        //             blocks = [
        //                 [
        //                     "type": "section",
        //                     "text": [
        //                         "type": "mrkdwn",
        //                         "text": "Terraform will perform the actions described above. Only 'Yes' will be accepted to approve:"
        //                     ]
        //                 ],                        
        //                 [
        //                     "type": "divider"
        //                 ],
        //                 [
        //                     "type": "actions",
        //                     "elements": [
        //                         [
        //                             "type": "button",
        //                             "text": [
        //                                 "type": "plain_text",
        //                                 "emoji": true,
        //                                 "text": "Yes"
        //                             ],
        //                             "style": "primary",
        //                             "url": "${BUILD_URL}/input/Confirm/proceedEmpty"
        //                         ],
        //                         [
        //                             "type": "button",
        //                             "text": [
        //                                 "type": "plain_text",
        //                                 "emoji": true,
        //                                 "text": "No"
        //                             ],
        //                             "style": "danger",
        //                             "url": "${BUILD_URL}/input/Confirm/abort"
        //                         ]
        //                     ]				
        //                 ]
        //             ]
        //         }
                
        //         slackSend(channel: "#terraform-approve", blocks: blocks)

        //         script {
        //             input id: 'confirm', message: 'Terraform will perform the actions described above. Only "Yes" will be accepted to approve:', ok: 'Yes', submitter: 'admin'
        //         }

        //         // sh "terraform apply -auto-approve -no-color"
        //         sh "terraform destroy -auto-approve -no-color"                
        //     }

        //     post {
        //         aborted{
        //             echo "Apply cancelled"
        //         }
        //     }            
        // }
    }

    post {
        cleanup { 
            cleanWs()
        }
    }    
}
