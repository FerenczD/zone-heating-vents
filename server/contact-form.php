<?php
$name = $_POST['name'];
$email = $_POST['email'];
$message = $_POST['message'];

$emailFrom = "Contact@BatesMobileDetailing.com";
$emailSubject = "Contact Form Submission";
$emailBody = "Name: $name.\n"."Email: $email.\n"."Message: $message.\n";
$emailTo = "bilyk.lyubomyr.lb@gmail.com";
$emailHeader = "From: $emailFrom \r\n";
$emailHeader.= "Reply-To: $email \r\n";

mail($emailTo, $emailSubject, $emailBody, $emailHeader);
header("Location contact.html");


