<?php

    /*
    * Project created by Numaan Chaudhry 29/11/2016
    * Demo Instructable for #IntelIOT
    * This script renders the data in the database into a simple dashboard of tiles and charts
    */

    require("common.php");

    // iterate through the results
    foreach ($sensorCollection->find() as $document) {
     //   echo "<pre>".(print_r($document, TRUE) . "\n")."</pre>";
    }

    $dashboardData = array();

    $idx=0;
    foreach( $staticCollection->find() as $doc )
        $dashboardData[] = array(
            "TileId"=>"tile".($idx++), 
            "Icon"=>"fa-".$doc["UIIcon"],
            "Title"=>$doc["Title"],
            "data"=>$doc["Data"]
        );
        

    $systemsToData = array(); //Will end up looking like array("Module 0"=>array(array("Date"=>..,"BattVoltage"=>...),array(..)))

    foreach($sensorCollection->find() as $doc ) {
        
        $serverTime = $doc["ServerTime"];
        $location   = $doc["Location"];
        $data       = json_decode($doc["Data"]);
        
        $moduleId   = $data->ModuleID;

        if (!array_key_exists($moduleId, $systemsToData))
            $systemsToData[$moduleId] = array();
        
        //'BattV':25.8, 'PVV':55.8, 'Temp':30}",
        $thisSys = $systemsToData[$moduleId];
        $thisSys[] = array(
            "BattV" =>$data->BattV,
            "PVV"   =>$data->PVV,
            "Ct"    =>$data->Ct,
            "Temp"  =>$data->Temp,
            "Location"=>$location
        );
        $systemsToData[$moduleId] = $thisSys;
        
    }
    
    $idx=0;
    $chartData = array();
    foreach($systemsToData as $moduleId=>$sysData) {

        $dataSeries = array();
        
        $batt   = array();
        $pv     = array();
        $ct     = array();
        $temp   = array();
        $location = null;
        foreach($sysData as $sysDataItem) {
            
            $bat[]  = $sysDataItem["BattV"];
            $pv[]   = $sysDataItem["PVV"];
            $ct[]   = $sysDataItem["Ct"];
            $temp[] = $sysDataItem["Temp"];
            $location = $sysDataItem["Location"];
        }
        
  	    $dataSeries = array();
        $dataSeries[] = array(
                "seriesType"      => 'line',
                "collectionAlias" => 'Battery (V)',
                "data"            => $batt
            );
        $dataSeries[] = array(
                "seriesType"      => 'line',
                "collectionAlias" => 'PV (V)',
                "data"            => $pv
            );
        $dataSeries[] = array(
                "seriesType"      => 'line',
                "collectionAlias" => 'Usage (mA)',
                "data"            => $ct
            );
        $dataSeries[] = array(
                "seriesType"      => 'line',
                "collectionAlias" => 'Temperature',
                "data"            => $temp
            );
        
        $chartData[] = 
            array("ChartId"=>"Chart".($idx++), "Title"=>$location, "DataSeries"=>$dataSeries);
    }

?>


<!DOCTYPE html>

<html>
<head>
        
	<title>Rural Remote Monitoring</title>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
    
    <script src="js/jquery-1.11.1.min.js"   type="text/javascript"></script>
    <script src="js/bootstrap.min.js"       type="text/javascript"></script>
    <script src="js/shieldui-all.min.js"    type="text/javascript"></script>
    
    <link rel="stylesheet" type="text/css" href="css/font-awesome-4.3.0/css/font-awesome.min.css" />
    <link rel="stylesheet" type="text/css" href="css/dark-bootstrap-gradient/all.min.css" />
    <link rel="stylesheet" type="text/css" href="css/bootstrap.min.css" />

    <link rel="stylesheet" type="text/css" href="css/plugins.css" />
    <link rel="stylesheet" type="text/css" href="css/themes/default.css" id="style_color"/>
    
    <link rel="stylesheet" type="text/css" href="css/style-metronic.css?v=1" />
    <link rel="stylesheet" type="text/css" href="css/style.css?v=1" />
    <link rel="stylesheet" type="text/css" href="css/style-responsive.css?v=1" />
    <link rel="stylesheet" type="text/css" href="css/custom.css?v=1" />
    
    
    
    <!-- This is an inline javascript file for charting -->
    <script type="text/javascript">

        <?php

            //Convert the php data arrays to JS arrays
            echo "var tileData = ". json_encode($dashboardData) . ";\n";
            echo "var chartData = ". json_encode($chartData) . ";\n";
        ?>

        $(function () {   

            UpdateChartsAndTiles();

         });

        //This creates javascript methods to build all charts and tiles
        function UpdateChartsAndTiles()
        {
            for(var i=0;i<chartData.length;i++) {

                var chartId = chartData[i].ChartId;
                var title = chartData[i].Title;
                var dataSeries = chartData[i].DataSeries; //Array of data series
                var color = i % 2 == 0 ? '#3ea7a0' : '#4884b8';

                CreateChart(color, chartId, dataSeries);
            }

            for(var i=0;i<tileData.length;i++) {

                $elem = $("#"+tileData[i].TileId);
                $elem.html(tileData[i].data);
            }
        }

        //Creates a ShieldUI chart
        function CreateChart(spColor, ChartId, dataSeries) //spColor, ChartId, XAxisData, SeriesData, TT) 
        {

           // $dbAnlyticData = array(10,20,30);
           // $months = array('December', 'January', 'February');

            SecHead = 'Countries';
            MaxY = 1000;
            br = '<br>';

            $("#" + ChartId).shieldChart({
               theme: "light",
                primaryHeader: {
                    text: ""
                },
                exportOptions: {
                    image: false,
                    print: false
                },
                axisY: [{
                    title: {
                        text: "Values (Volts/milliAmps/0C)",
                        style: {
                            color: '#4DB0F5'
                        }
                    },
                    axisTickText: {
                        style: {
                            color: '#4DB0F5'
                        }
                    }
                }],
                seriesSettings: {
                    line: {
                        enablePointSelection: true
                    }
                },
                tooltipSettings: {
                    axisMarkers: {
                        enabled: true,
                        mode: 'x',
                        width: 1,
                        zIndex: 3
                    }
                },
                dataSeries: dataSeries,
                events: {
                    seriesClick: function (args) {
                        log("seriesClick on series " + args.dataSeries.collectionAlias);
                    },
                    pointSelect: function (args) {
                        log("pointSelect on series " + args.dataSeries.collectionAlias + ", point " + args.point.name);
                    }
                }
            });
        }
  
 
</script>
</head>
    
    
<body class="page-header-fixed page-sidebar-fixed">
    
<!-- BEGIN HEADER -->
<div class="header navbar navbar-fixed-top">
	<!-- BEGIN TOP NAVIGATION BAR -->
	<div class="header-inner">
		<!-- BEGIN LOGO -->
	    <a class="navbar-brand"  >
			<img src="images/icon.png" alt="logo" width="150px" class="img-responsive" style="width:25px"/>
		</a>

		<!-- END LOGO -->
		<!-- BEGIN RESPONSIVE MENU TOGGLER -->
		<a href="javascript:;" class="navbar-toggle" data-toggle="collapse" data-target=".navbar-collapse">
			<img src="img/menu-toggler.png" alt=""/>
		</a>
		<!-- END RESPONSIVE MENU TOGGLER -->
		<!-- BEGIN TOP NAVIGATION MENU -->
		<ul class="nav navbar-nav pull-right">
			<!-- BEGIN NOTIFICATION DROPDOWN -->

			<!-- END NOTIFICATION DROPDOWN -->
			<!-- BEGIN INBOX DROPDOWN -->
	

			<!-- END INBOX DROPDOWN -->
			<!-- BEGIN TODO DROPDOWN -->
			<li class="dropdown" id="header_task_bar">
				
                <!--
				<ul class="dropdown-menu extended tasks">
					<li>
						<p>
							 You have 12 pending tasks
						</p>
					</li>
					<li>

						<ul class="dropdown-menu-list scroller" style="height: 250px;">
							<li>
								<a href="#">
									<span class="task">
										<span class="desc">
											 New release v1.2
										</span>
										<span class="percent">
											 30%
										</span>
									</span>
									<span class="progress">
										<span style="width: 40%;" class="progress-bar progress-bar-success" aria-valuenow="40" aria-valuemin="0" aria-valuemax="100">
											<span class="sr-only">
												 40% Complete
											</span>
										</span>
									</span>
								</a>
							</li>
							<li>
								<a href="#">
									<span class="task">
										<span class="desc">
											 Application deployment
										</span>
										<span class="percent">
											 65%
										</span>
									</span>
									<span class="progress progress-striped">
										<span style="width: 65%;" class="progress-bar progress-bar-danger" aria-valuenow="65" aria-valuemin="0" aria-valuemax="100">
											<span class="sr-only">
												 65% Complete
											</span>
										</span>
									</span>
								</a>
							</li>
							<li>
								<a href="#">
									<span class="task">
										<span class="desc">
											 Mobile app release
										</span>
										<span class="percent">
											 98%
										</span>
									</span>
									<span class="progress">
										<span style="width: 98%;" class="progress-bar progress-bar-success" aria-valuenow="98" aria-valuemin="0" aria-valuemax="100">
											<span class="sr-only">
												 98% Complete
											</span>
										</span>
									</span>
								</a>
							</li>
							<li>
								<a href="#">
									<span class="task">
										<span class="desc">
											 Database migration
										</span>
										<span class="percent">
											 10%
										</span>
									</span>
									<span class="progress progress-striped">
										<span style="width: 10%;" class="progress-bar progress-bar-warning" aria-valuenow="10" aria-valuemin="0" aria-valuemax="100">
											<span class="sr-only">
												 10% Complete
											</span>
										</span>
									</span>
								</a>
							</li>
							<li>
								<a href="#">
									<span class="task">
										<span class="desc">
											 Web server upgrade
										</span>
										<span class="percent">
											 58%
										</span>
									</span>
									<span class="progress progress-striped">
										<span style="width: 58%;" class="progress-bar progress-bar-info" aria-valuenow="58" aria-valuemin="0" aria-valuemax="100">
											<span class="sr-only">
												 58% Complete
											</span>
										</span>
									</span>
								</a>
							</li>
							<li>
								<a href="#">
									<span class="task">
										<span class="desc">
											 Mobile development
										</span>
										<span class="percent">
											 85%
										</span>
									</span>
									<span class="progress progress-striped">
										<span style="width: 85%;" class="progress-bar progress-bar-success" aria-valuenow="85" aria-valuemin="0" aria-valuemax="100">
											<span class="sr-only">
												 85% Complete
											</span>
										</span>
									</span>
								</a>
							</li>
							<li>
								<a href="#">
									<span class="task">
										<span class="desc">
											 New UI release
										</span>
										<span class="percent">
											 18%
										</span>
									</span>
									<span class="progress progress-striped">
										<span style="width: 18%;" class="progress-bar progress-bar-important" aria-valuenow="18" aria-valuemin="0" aria-valuemax="100">
											<span class="sr-only">
												 18% Complete
											</span>
										</span>
									</span>
								</a>
							</li>
						</ul>

					</li>
					<li class="external">
						<a href="#">
							 See all tasks <i class="m-icon-swapright"></i>
						</a>
					</li>
				</ul>
-->
			
			<!-- END TODO DROPDOWN -->
			<!-- BEGIN USER LOGIN DROPDOWN -->
			<li class="dropdown user">
				<a href="#" class="dropdown-toggle" data-toggle="dropdown" data-hover="dropdown" data-close-others="true">
<!--					<img alt="" src="/assets/img/avatar1_small.jpg"/>-->
					<span class="username">
				        <?php echo "Numaan Chaudhry" . " | " . "SysAdmin"; ?>
					</span>
					<i class="fa fa-angle-down"></i>
				</a>
				<ul class="dropdown-menu">

					<li>
						<a href="javascript:;" id="trigger_fullscreen">
							<i class="fa fa-arrows"></i> Full Screen
						</a>
					</li>
					<li>
						<a href="javascript:beginLogout();">
							<i class="fa fa-key"></i> Log Out
						</a>
					</li>
				</ul>
			</li>
			<!-- END USER LOGIN DROPDOWN -->
		</ul>
		<!-- END TOP NAVIGATION MENU -->
	</div>
	<!-- END TOP NAVIGATION BAR -->
</div>
<!-- END HEADER -->
<div class="clearfix">
</div>
<!-- BEGIN CONTAINER -->
<div class="page-container">
	<!-- BEGIN SIDEBAR -->
	<div class="page-sidebar-wrapper">
		<div class="page-sidebar navbar-collapse collapse">
			<!-- BEGIN SIDEBAR MENU -->
			<ul class="page-sidebar-menu">
				<li class="sidebar-toggler-wrapper">
					<!-- BEGIN SIDEBAR TOGGLER BUTTON -->
					<div class="sidebar-toggler hidden-phone">
					</div>
					<!-- BEGIN SIDEBAR TOGGLER BUTTON -->
				</li>
				<li class="sidebar-search-wrapper">
					<!-- BEGIN RESPONSIVE QUICK SEARCH FORM -->
					<form class="sidebar-search" action="extra_search.html" method="POST">
						<div class="form-container">
							<div class="input-box">
								<a href="javascript:;" class="remove">
								</a>
<!--
								<input type="text" placeholder="Search..."/>
								<input type="button" class="submit" value=" "/>
-->
							</div>
						</div>
					</form>
					<!-- END RESPONSIVE QUICK SEARCH FORM -->
				</li>
				<li class="start">
					<a href="/index.php">
						<i class="fa fa-home"></i>
						<span class="title">
							Benin
						</span>
						<span class="selected">
						</span>
						<span class="arrow closed">
						</span>
					</a>
				</li>
				<li class="start">
					<a href="/index.php">
						<i class="fa fa-home"></i>
						<span class="title">
							Mali
						</span>
						<span class="arrow closed">
						</span>
					</a>
				</li>
				<li class="start active">
					<a href="/index.php">
						<i class="fa fa-home"></i>
						<span class="title">
							Sierra Leone
						</span>
						<span class="arrow open">
						</span>
					</a>
				</li>
				
  
			</ul>
			<!-- END SIDEBAR MENU -->
		</div>
	</div>
	<!-- END SIDEBAR -->
	<!-- BEGIN CONTENT -->
	<div class="page-content-wrapper">
		<div class="page-content">
			<!-- BEGIN SAMPLE PORTLET CONFIGURATION MODAL FORM-->
			<div class="modal fade" id="portlet-config" tabindex="-1" role="dialog" aria-labelledby="myModalLabel" aria-hidden="true">
				<div class="modal-dialog">
					<div class="modal-content">
						<div class="modal-header">
							<button type="button" class="close" data-dismiss="modal" aria-hidden="true"></button>
							<h4 class="modal-title">Modal title</h4>
						</div>
						<div class="modal-body">
							 Widget settings form goes here
						</div>
						<div class="modal-footer">
							<button type="button" class="btn blue">Save changes</button>
							<button type="button" class="btn default" data-dismiss="modal">Close</button>
						</div>
					</div>
					<!-- /.modal-content -->
				</div>
				<!-- /.modal-dialog -->
			</div>

			<!-- END STYLE CUSTOMIZER -->
			<!-- BEGIN PAGE HEADER-->
			<div class="row">
				<div class="col-md-12">
					<!-- BEGIN PAGE TITLE & BREADCRUMB-->
					<h3 class="page-title">
					CARS <small>Central Analytics and Reporting System</small>
					</h3>
					
					<ul class="page-breadcrumb breadcrumb">
						<?php
							$breadcrumbs = explode("/", $_SERVER["REQUEST_URI"]); //Gets breadcrumb items from the url

							//Prints the home icon at beggining of the breadcrumb trail.							
							echo "<li><i class=\"fa fa-home\"></i><a href=\"#\">Home</a><i class=\"fa fa-angle-right\"></i></li>";

                            echo "<li><i></i><a href=\"#\">Dashboard</a>";

						?>	
                    </ul>
                    
					<!-- END PAGE TITLE & BREADCRUMB-->
				</div>
			</div>
			<!-- END PAGE HEADER-->
			<!-- BEGIN PAGE CONTENT-->
			<div class="row">
				<div class="col-md-12">
					 
    
    

                    <div class="row">
                        <?php 
                            $counter=0;
                            foreach ($dashboardData as $dataItem) {
                                $color = ($counter++ % 2 == 0) ? "blue-madison" : "green-haze";
                        ?>
                        <div class="col-lg-3 col-md-3 col-sm-6 col-xs-12">
                            <div class="dashboard-stat <?php echo $color ?>">
                                <div class="visual">
                                    <i class="fa <?php echo $dataItem["Icon"]; ?>"></i>
                                </div>
                                <div class="details">
                                    <div id="<?php echo $dataItem["TileId"]; ?>" class="number">
                                         Loading...
                                    </div>
                                    <div class="desc">
                                         <?php echo $dataItem["Title"] ?>
                                    </div>
                                </div>
                                <a class="more" href="#">
                    <!--				 View more <i class="m-icon-swapright m-icon-white"></i>-->
                                </a>
                            </div>
                        </div>
                        <?php } ?>
                    </div>


                    <div class="row">
                        <?php 
                            foreach($chartData as $chartItem) {
                                $no = $chartItem["ChartId"];
                                
                        ?>
                        <div class="col-md-12 col-sm-12"> <!-- Half-Width Column -->
                            <!-- BEGIN PORTLET-->
                            <div class="portlet solid bordered">
                                <div class="portlet-title">
                                    <div class="caption">
                                        <i class="fa fa-bar-chart-o"></i><?php echo $chartItem["Title"]; ?>
                                    </div>
                                </div>
                                <div class="portlet-body">
                                    <div id="site_statistics_loading" style="display: none;">
                                        <img src="assets/img/loading.gif" alt="loading">
                                    </div>
                                    <div id="<?php echo $no;?>Container" class="display-none" style="display: block;">
                                        <div id="<?php echo $no;?>" class="chart" style="padding: 0px; position: relative;"></div>
                                    </div>
                                </div>
                            </div>
                            <!-- END PORTLET-->
                        </div>
                        <?php } ?>

                    </div>



				</div>
			</div>
			<!-- END PAGE CONTENT-->
		</div>
	</div>
	<!-- END CONTENT -->
</div>
<!-- END CONTAINER -->

</body>
</html>

