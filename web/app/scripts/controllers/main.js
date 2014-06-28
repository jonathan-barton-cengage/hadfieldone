'use strict';

angular.module('jonathan-barton-cengage.Hadfieldone')

  .controller('MainCtrl', function($scope, $location, version) {

    $scope.$path = $location.path.bind($location);
    $scope.version = version;
    $scope.projectTitle = "Project Hadfield";
    $scope.mainMenuItems = [
        {
            
        }
    ]

  });
