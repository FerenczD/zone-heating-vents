<?php
require_once 'functions.php';
require_once 'dbconfig.php';

$action = filter_input(INPUT_POST, 'action');

switch ($action) {
    case 'thermSendTemp':
	$temp = filter_input(INPUT_POST,'temp');
	$mac = filter_input(INPUT_POST,'mac');
        echo thermSendTemp($temp,$mac);
        break;
    case 'thermRequestTemp':
        $mac = filter_input(INPUT_POST,'mac');
        echo thermRequestTemp($mac);
        break;
    default:
        echo null;
        break;
}