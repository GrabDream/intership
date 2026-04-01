<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
$page_name = "m_systemadmin";
include_once($_SERVER ["DOCUMENT_ROOT"] . "/authenticed.php");
include_once("user_functions.php");
require_once($_SERVER["DOCUMENT_ROOT"] . "/model/common_param.php");
isAvailable('m_systemadmin');
$user_name = htmlspecialchars($_GET["user_name"], ENT_QUOTES, 'UTF-8');
$tokenid = $_GET["tokenid"];
$apiclass = new ApiInter();

$ret = $apiclass->setApiInterData("/netmanage/userauth/UserTotp/getTotpInfo");
$totpInfo = $ret["result"];
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
    <title>Google Authenticator <?= _gettext('verify'); ?></title>
    <link href="/css/common.css" rel="stylesheet" type="text/css"/>
    <link href="/css/skin.css" rel="stylesheet" type="text/css"/>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
    <script src="/js/qrcode.min.js"></script>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        }

        .container {
            width: 100%;
            overflow: hidden;
            animation: fadeIn 0.5s ease-out;
        }

        @keyframes fadeIn {
            from {
                opacity: 0;
                transform: translateY(-20px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }

        .logo {
            position: absolute;
            top: 20px;
            left: 20px;
            font-size: 24px;
        }

        .steps {
            display: flex;
            justify-content: space-between;
            padding: 20px;
            margin-top: 10px;
        }

        .step {
            display: flex;
            flex-direction: column;
            align-items: center;
            position: relative;
            flex: 1;
        }

        .step:not(:last-child):after {
            content: '';
            position: absolute;
            top: 20px;
            right: -50%;
            width: 100%;
            height: 2px;
            background: #ddd;
            z-index: 1;
        }

        .step.active:not(:last-child):after {
            /*background: #3C69B5;*/
        }

        .step.completed:not(:last-child):after {
            background: #34a853;
        }

        .step-icon {
            width: 40px;
            height: 40px;
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            background: #ddd;
            color: #777;
            margin-bottom: 8px;
            z-index: 2;
            font-size: 18px;
        }

        .step.active .step-icon {
            background: #3C69B5;
            color: white;
        }

        .step.completed .step-icon {
            background: #34a853;
            color: white;
        }

        .step-label {
            font-size: 12px;
            color: #777;
            text-align: center;
        }

        .step.active .step-label {
            color: #3C69B5;
            font-weight: 600;
        }

        .content {
            padding: 0 20px;
        }

        .step-content {
            display: none;
        }

        .step-content.active {
            display: block;
            animation: fadeIn 0.5s ease-out;
        }

        .form-group label {
            display: block;
            margin-bottom: 8px;
            font-weight: 500;
            color: #333;
        }

        .qr-container {
            margin: 20px 0;
            display: flex;
            gap: 20px;
        }

        .qr-code {
            text-align: center;
            min-width: 200px;
            height: 200px;
            background: #f5f5f5;
            border-radius: 8px;
            display: flex;
            align-items: center;
            justify-content: center;
            border: 1px solid #ddd;
            position: relative;
            overflow: hidden;
        }

        .qr-code:before {
            content: '';
            position: absolute;
            width: 100%;
            height: 100%;
            background: linear-gradient(45deg, transparent 45%, #3C69B5 46%, #3C69B5 54%, transparent 55%),
            linear-gradient(-45deg, transparent 45%, #3C69B5 46%, #3C69B5 54%, transparent 55%);
            background-size: 20px 20px;
            opacity: 0.1;
        }

        .qr-placeholder {
            font-size: 14px;
            color: #777;
        }

        .key-display {
            width: 490px;
            background: #f8f9fa;
            padding: 15px 5px;
            border-radius: 8px;
            text-align: center;
            margin: 8px 0px 20px 0;
            border: 1px dashed #ddd;
            font-size: 13px;
            letter-spacing: 1px;
            color: #333;
        }

        .instructions {
            background: #e8f4fd;
            padding: 15px;
            border-radius: 8px;
            font-size: 14px;
            color: #3C69B5;
            border-left: 4px solid #3C69B5;
        }

        .instructions ol {
            margin-left: 20px;
        }

        .instructions li {
            margin-bottom: 8px;
        }

        .success-message {
            text-align: center;
            padding: 20px;
            margin-top: 20px;
        }

        .success-icon {
            font-size: 60px;
            color: #34a853;
            margin-bottom: 20px;
        }

        .success-message h3 {
            color: #34a853;
            margin-bottom: 10px;
        }

        .timer {
            text-align: center;
            margin-top: 10px;
            font-size: 14px;
            color: #777;
        }

        .timer span {
            font-weight: bold;
            color: #3C69B5;
        }

        .verifyCode {
            height: 67px !important;
            width: 194px;
            font-size: 20px;
            float: right;
            margin: 0;
        }

        .dynamicCode {
            width: 100%;
            padding: 12px 15px;
            border: 1px solid #ddd;
            border-radius: 8px;
            transition: all 0.3s;
            height: 67px !important;
            font-size: 40px !important;
        }

        .dynamicCode:focus {
            border-color: #3C69B5;
            box-shadow: 0 0 0 2px rgba(66, 133, 244, 0.2);
            outline: none;
        }

        .qr-code canvas {
            border-radius: 8px;
        }
    </style>
    <script type="text/javascript" src="/jqueryjs/jquery.min.js"></script>
    <script type="text/javascript" src="/js/prototype.js"></script>
</head>
<body>
<div class="container">
    <div class="steps">
        <div class="step">
            <div class="step-icon">
                <i class="fas fa-mobile-alt"></i>
            </div>
            <div class="step-label"><?= _gettext("bind_verify"); ?></div>
        </div>
        <div class="step">
            <div class="step-icon">
                <i class="fas fa-check"></i>
            </div>
            <div class="step-label"><?= _gettext("complete"); ?></div>
        </div>
    </div>

    <div class="content">

        <div class="step-content active" id="step2">
            <div class="instructions">
                <p><strong>Google AuthenticatorAPP <?= _gettext("totp_bind_msg"); ?></strong></p>
            </div>

            <div class="qr-container">
                <div class="qr-code">
                    <div class="qr-placeholder" id="qrcode-container">
                        <?php if (!$totpInfo['qrUrl']) { ?>
                            <i class="fas fa-qrcode" style="font-size: 40px; margin-bottom: 10px; display: block;"></i>
                            Google Authenticator <?= _gettext("qrcode"); ?>
                        <?php } ?>
                    </div>
                </div>
                <div class="secret-code">
                    <div class="form-group">
                        <label><?= _gettext("secret"); ?>：</label>
                        <div class="key-display" id="secret"><span><?= $totpInfo['secret'] ?></span></div>
                    </div>
                    <div class="form-group">
                        <label for="dynamicCode"><?= _gettext("dynamic_code_msg"); ?>：</label>
                        <div>
                            <input type="text" id="dynamicCode" class="dynamicCode" maxlength="6">
                            <button class="btn_ok verifyCode" id="verifyCode"><?= _gettext("verify_now"); ?></button>
                        </div>
                    </div>
                </div>
            </div>
        </div>

        <div class="step-content" id="step3">
            <div class="success-message">
                <div class="success-icon">
                    <i class="fas fa-check-circle"></i>
                </div>
                <h3><?= _gettext("verify_success"); ?>！</h3>
                <p>
                    <?= _gettext("verify_success"); ?>：<span><?php echo $user_name; ?></span>
                    TOTP <?= _gettext("verify_success"); ?>
                </p>
            </div>
        </div>
    </div>
</div>
</body>
<script>
    document.addEventListener('DOMContentLoaded', function () {
        const step2 = $('step2');
        const step3 = $('step3');
        const verifyCodeBtn = $('verifyCode');
        const steps = document.querySelectorAll('.step');
        const dynamicCodeInput = $('dynamicCode');

        function generateQRCode() {
            const qrUrl = '<?= $totpInfo['qrUrl'] ?>';
            console.log(qrUrl);
            if (qrUrl) {
                const qrContainer = $('qrcode-container');
                qrContainer.innerHTML = '';
                new QRCode(qrContainer, {
                    text: qrUrl,
                    width: 200,
                    height: 200,
                    colorDark: "#000000",
                    colorLight: "#ffffff",
                    correctLevel: QRCode.CorrectLevel.H
                });
            }
        }

        function goToStep(stepNumber) {
            step2.classList.remove('active');
            step3.classList.remove('active');

            if (stepNumber === 2) {
                step2.classList.add('active');
            } else if (stepNumber === 3) {
                step3.classList.add('active');
            }

            steps.forEach((step, index) => {
                step.classList.remove('active', 'completed');

                if (stepNumber === 2) {
                    if (index === 0) {
                        step.classList.add('active');
                    }
                } else if (stepNumber === 3) {
                    if (index === 0 || index === 1) {
                        step.classList.add('completed');
                    }
                }
            });
        }

        verifyCodeBtn.addEventListener('click', function () {
            const code = dynamicCodeInput.value.trim();
            const secret = $('secret').down('span').innerHTML;

            if (!code || code.length !== 6) {
                alert('<?= _gettext("please_enter_6_digit_code"); ?>');
                return;
            }

            if (!/^\d+$/.test(code)) {
                alert('<?= _gettext("dynamic_code_must_be_numbers"); ?>');
                return;
            }

            const originalText = verifyCodeBtn.innerHTML;

            verifyCodeBtn.disabled = true;
            verifyCodeBtn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> <?= _gettext("verifying"); ?>...';

            new Ajax.Request('user_totp_commit.php', {
                method: 'post',
                parameters: {
                    tokenid: '<?php echo $tokenid; ?>',
                    user_name: '<?php echo $user_name; ?>',
                    secret: secret,
                    code: code,
                    action: 1
                },
                onSuccess: function (transport) {
                    const response = transport.responseText.evalJSON();
                    if (response.code === 0){
                        goToStep(3);
                        let parentWindow = window.parent;
                        parentWindow.document.getElementById('totp_secret').value = secret;
                        parentWindow.showTotp();
                    } else {
                        alert(response.message);
                    }
                },
                onFailure: function () {
                    alert('<?= _gettext("verifiy_fail"); ?>');
                },
                onComplete: function () {
                    verifyCodeBtn.disabled = false;
                    verifyCodeBtn.innerHTML = originalText;
                }
            });
        });

        dynamicCodeInput.addEventListener('keypress', function (event) {
            if (event.key === 'Enter') {
                verifyCodeBtn.click();
            }
        });

        generateQRCode();
        dynamicCodeInput.focus();
        goToStep(2)
    });
</script>
</html>